#include "Mesh.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <filesystem>

// Lock the attribute locations we’ll use everywhere
static constexpr GLuint ATTR_POS = 0;
static constexpr GLuint ATTR_UV  = 1;  // << UV at 1
static constexpr GLuint ATTR_NRM = 2;  // << normal at 2
static constexpr GLuint ATTR_COL = 3;  // << material colour at 3

// ----------------------------
// Mesh: lifecycle
// ----------------------------
Mesh::Mesh() {}

Mesh::~Mesh() { ClearMesh(); }

// ----------------------------
// Create from raw arrays (8 floats per vertex)
// ----------------------------
void Mesh::CreateMesh(const GLfloat* vertices,
                    const unsigned int* indices,
                    unsigned int numOfFloats,
                    unsigned int numOfIndices) 
{
    // Build vectors and forward to preferred path
    std::vector<VertexPNU> vtx;

    if (numOfFloats % 8 == 0) 
    {
        // Layout: P(3) N(3) UV(2)
        unsigned int numVerts = numOfFloats / 8;
        vtx.reserve(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i) 
        {
            const GLfloat* base = vertices + i * 8;
            VertexPNU v{};
            v.position = { base[0], base[1], base[2] };
            v.normal   = { base[3], base[4], base[5] };
            v.uv       = { base[6], base[7] };
            vtx.push_back(v);
        }
    }
    else if (numOfFloats % 5 == 0) 
    {
        // Layout: P(3) UV(2) — normals default to (0,0,1)
        unsigned int numVerts = numOfFloats / 5;
        vtx.reserve(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i) 
        {
            const GLfloat* base = vertices + i * 5;
            VertexPNU v{};
            v.position = { base[0], base[1], base[2] };
            v.normal   = { 0.0f, 0.0f, 1.0f };
            v.uv       = { base[3], base[4] };
            vtx.push_back(v);
        }
    }
    else 
    {
        std::cerr << "[Mesh] Error: unsupported vertex float count " 
                  << numOfFloats << "\\n";
        return;
    }

    std::vector<unsigned int> idx(indices, indices + numOfIndices);
    CreateMesh(vtx, idx);
}

// ----------------------------
// Create from std::vector (preferred)
// ----------------------------
void Mesh::CreateMesh(const std::vector<VertexPNU>& vertices,
                    const std::vector<unsigned int>& indices) 
{
    uploadInterleaved_(vertices, indices);
}

// ----------------------------
// Upload helper (interleaved VBO + EBO)
// ----------------------------
void Mesh::uploadInterleaved_(const std::vector<VertexPNU>& vertices,
                            const std::vector<unsigned int>& indices) 
{
    // Clean previous
    ClearMesh();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexPNU)),
                vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                indices.data(), GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(sizeof(VertexPNU));

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(VertexPNU,position)); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(VertexPNU,uv));       glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(VertexPNU,normal));   glEnableVertexAttribArray(2);
    glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(VertexPNU,color));    glEnableVertexAttribArray(3);


    indexCount = static_cast<GLsizei>(indices.size());

    // unbind VAO (EBO remains associated with VAO)
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ----------------------------
// .OBJ loader (robust tuples + dedup + material colours + cache)
// ----------------------------
namespace
{
    struct Triple
    {
        int v, vt, vn, m;  // already fixed: 0-based or -1, m = material index
        bool operator==(const Triple& o) const { return v==o.v && vt==o.vt && vn==o.vn && m==o.m; }
    };
    struct TripleHash
    {
        size_t operator()(const Triple& t) const
        {
            return (size_t)((t.v+1)*73856093) ^ (size_t)((t.vt+1)*19349663)
                 ^ (size_t)((t.vn+1)*83492791) ^ (size_t)((t.m+1)*2654435761u);
        }
    };

    // Blender writes Kd in linear colour space, but our shaders treat colours like
    // texture pixels (sRGB, as shown on screen) -> convert, or everything looks too dark
    inline float LinearToSRGB(float c)
    {
        c = std::fmax(0.0f, std::fmin(1.0f, c));
        return (c <= 0.0031308f) ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
    }

    // Read "newmtl name" + "Kd r g b" pairs from a .mtl file
    std::unordered_map<std::string, glm::vec3> LoadMTL(const std::filesystem::path& mtlPath)
    {
        std::unordered_map<std::string, glm::vec3> colours;
        std::ifstream in(mtlPath);
        if (!in)
        {
            std::cerr << "[Mesh] MTL not found (using white): " << mtlPath.u8string() << "\n";
            return colours;
        }

        std::string line, current;
        while (std::getline(in, line))
        {
            std::istringstream ss(line);
            std::string tag; ss >> tag;
            if (tag == "newmtl") { ss >> current; colours[current] = glm::vec3(1.0f); }
            else if (tag == "Kd" && !current.empty())
            {
                glm::vec3 kd(1.0f); ss >> kd.r >> kd.g >> kd.b;
                colours[current] = glm::vec3(LinearToSRGB(kd.r), LinearToSRGB(kd.g), LinearToSRGB(kd.b));
            }
        }
        return colours;
    }

    // ---- binary cache: <obj>.meshcache = magic, counts, raw vertex + index arrays ----
    // Bump the version whenever the loader changes how vertices are built (old caches get rebuilt)
    constexpr char CACHE_MAGIC[8] = { 'M','E','S','H','C','0','0','2' };

    std::filesystem::path CachePathFor(const std::filesystem::path& objPath)
    {
        std::filesystem::path p = objPath;
        p += ".meshcache";
        return p;
    }

    bool ReadCache(const std::filesystem::path& cachePath,
                   std::vector<VertexPNU>& vertices, std::vector<unsigned int>& indices)
    {
        std::ifstream in(cachePath, std::ios::binary);
        if (!in) return false;

        char magic[8]{}; uint64_t nv = 0, ni = 0;
        in.read(magic, 8);
        in.read(reinterpret_cast<char*>(&nv), sizeof(nv));
        in.read(reinterpret_cast<char*>(&ni), sizeof(ni));
        if (!in || std::memcmp(magic, CACHE_MAGIC, 8) != 0 || nv == 0 || ni == 0) return false;

        vertices.resize(nv);
        indices.resize(ni);
        in.read(reinterpret_cast<char*>(vertices.data()), nv * sizeof(VertexPNU));
        in.read(reinterpret_cast<char*>(indices.data()), ni * sizeof(unsigned int));
        return static_cast<bool>(in);
    }

    void WriteCache(const std::filesystem::path& cachePath,
                    const std::vector<VertexPNU>& vertices, const std::vector<unsigned int>& indices)
    {
        std::ofstream out(cachePath, std::ios::binary);
        if (!out) return;
        uint64_t nv = vertices.size(), ni = indices.size();
        out.write(CACHE_MAGIC, 8);
        out.write(reinterpret_cast<const char*>(&nv), sizeof(nv));
        out.write(reinterpret_cast<const char*>(&ni), sizeof(ni));
        out.write(reinterpret_cast<const char*>(vertices.data()), nv * sizeof(VertexPNU));
        out.write(reinterpret_cast<const char*>(indices.data()), ni * sizeof(unsigned int));
    }

    // Cache is valid only if it is newer than the .obj and the .mtl files next to it
    bool CacheIsFresh(const std::filesystem::path& objPath, const std::filesystem::path& cachePath)
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(cachePath, ec)) return false;
        auto cacheTime = fs::last_write_time(cachePath, ec);
        if (ec) return false;

        if (fs::last_write_time(objPath, ec) > cacheTime) return false;
        for (const auto& entry : fs::directory_iterator(objPath.parent_path(), ec))
        {
            if (entry.path().extension() == ".mtl" && entry.last_write_time(ec) > cacheTime)
                return false;
        }
        return true;
    }

    // Parse floats after the tag (much faster than istringstream on huge files)
    inline void ParseFloats(const char* s, float* out, int count)
    {
        char* end = nullptr;
        for (int i = 0; i < count; ++i)
        {
            out[i] = std::strtof(s, &end);
            if (end == s) { out[i] = 0.0f; }
            s = end;
        }
    }
}

bool Mesh::CreateMeshFromOBJ(const std::string& path, bool flipV)
{
    const std::filesystem::path objPath = std::filesystem::u8path(path);
    const std::filesystem::path cachePath = CachePathFor(objPath);

    std::vector<VertexPNU> interleaved;
    std::vector<unsigned int> indices;

    if (CacheIsFresh(objPath, cachePath) && ReadCache(cachePath, interleaved, indices))
    {
        std::cout << "[Mesh] Loaded cache " << cachePath.filename().u8string()
                  << " (" << interleaved.size() << " vertices)\n";
        uploadInterleaved_(interleaved, indices);
        return true;
    }

    std::ifstream in(objPath);
    if (!in)
    {
        std::cerr << "[Mesh] Failed to open OBJ: " << path << "\n";
        return false;
    }
    std::cout << "[Mesh] Parsing " << objPath.filename().u8string() << " ...\n";

    std::vector<glm::vec3> V; V.reserve(1000);
    std::vector<glm::vec2> VT; VT.reserve(1000);
    std::vector<glm::vec3> VN; VN.reserve(1000);

    interleaved.reserve(2000);
    indices.reserve(6000);
    std::unordered_map<Triple, unsigned int, TripleHash> dedup;

    std::unordered_map<std::string, glm::vec3> mtlColours;
    std::vector<glm::vec3> materialColours;            // index = Triple::m
    std::unordered_map<std::string, int> materialIndex;
    int currentMaterial = -1;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        if (line.compare(0, 2, "v ") == 0)
        {
            glm::vec3 p{}; ParseFloats(line.c_str() + 2, &p.x, 3); V.push_back(p);
            continue;
        }
        if (line.compare(0, 3, "vt ") == 0)
        {
            glm::vec2 t{}; ParseFloats(line.c_str() + 3, &t.x, 2);
            if (flipV) t.y = 1.0f - t.y;
            VT.push_back(t);
            continue;
        }
        if (line.compare(0, 3, "vn ") == 0)
        {
            glm::vec3 n{}; ParseFloats(line.c_str() + 3, &n.x, 3); VN.push_back(n);
            continue;
        }

        std::istringstream ss(line);
        std::string tag; ss >> tag;
        if (tag == "mtllib")
        {
            std::string name; std::getline(ss >> std::ws, name);
            if (!name.empty() && name.back() == '\r') name.pop_back();
            auto loaded = LoadMTL(objPath.parent_path() / std::filesystem::u8path(name));
            mtlColours.insert(loaded.begin(), loaded.end());
        }
        else if (tag == "usemtl")
        {
            std::string name; ss >> name;
            auto it = materialIndex.find(name);
            if (it == materialIndex.end())
            {
                auto found = mtlColours.find(name);
                materialColours.push_back(found != mtlColours.end() ? found->second : glm::vec3(1.0f));
                it = materialIndex.emplace(name, (int)materialColours.size() - 1).first;
            }
            currentMaterial = it->second;
        }
        else if (tag == "f")
        {
            // collect the rest of tokens for this face
            std::vector<std::string> fverts; fverts.reserve(8);
            std::string tok;
            while (ss >> tok) { fverts.push_back(tok); }
            if (fverts.size() < 3) continue; // not a valid face

            auto parseOne = [&](const std::string& s, int& iv, int& ivt, int& ivn) -> bool
            {
                iv = ivt = ivn = -1;
                int parts[3] = {0,0,0}, p = 0;
                std::string cur;
                for (size_t i=0;i<=s.size();++i)
                {
                    if (i==s.size() || s[i]=='/')
                    {
                        if (!cur.empty()) { try { parts[p] = std::stoi(cur); } catch (...) { parts[p] = 0; } }
                        cur.clear(); if (i!=s.size() && p<2) ++p;
                    }
                    else cur.push_back(s[i]);
                }
                auto fixIndex = [](int idx, int count){ if (idx>0) return idx-1; if (idx<0) return count+idx; return -1; };
                iv  = fixIndex(parts[0], (int)V.size());
                ivt = (p>=1) ? fixIndex(parts[1], (int)VT.size()) : -1;
                ivn = (p>=2) ? fixIndex(parts[2], (int)VN.size()) : -1;
                return (iv >= 0); // v is mandatory
            };

            auto addFaceVertex = [&](int iv, int ivt, int ivn)
            {
                Triple key{iv, ivt, ivn, currentMaterial};
                auto it = dedup.find(key);
                if (it != dedup.end()) { indices.push_back(it->second); return; }

                VertexPNU out{};
                out.position = V[iv];
                if (ivt >= 0) out.uv = VT[ivt];
                if (ivn >= 0) out.normal = VN[ivn];
                if (currentMaterial >= 0) out.color = materialColours[currentMaterial];

                unsigned int id = (unsigned)interleaved.size();
                interleaved.push_back(out);
                dedup.emplace(key, id);
                indices.push_back(id);
            };

            // Triangulate via fan: (v0, vi, vi+1)
            // First, parse all vertices of this polygon to a temp list of indices
            struct FaceV { int v, vt, vn; };
            std::vector<FaceV> fv; fv.reserve(fverts.size());
            for (const auto& s : fverts)
            {
                int iv=0, ivt=0, ivn=0;
                parseOne(s, iv, ivt, ivn);
                fv.push_back({iv, ivt, ivn});
            }
            for (size_t i = 1; i + 1 < fv.size(); ++i)
            {
                // triangle: fv[0], fv[i], fv[i+1]
                addFaceVertex(fv[0].v, fv[0].vt, fv[0].vn);
                addFaceVertex(fv[i].v, fv[i].vt, fv[i].vn);
                addFaceVertex(fv[i+1].v, fv[i+1].vt, fv[i+1].vn);
            }
        }
    }

    if (interleaved.empty() || indices.empty())
    {
        std::cerr << "[Mesh] OBJ produced no geometry: " << path << "\n";
        return false;
    }

    std::cout << "[Mesh] " << interleaved.size() << " vertices, "
              << indices.size() / 3 << " triangles, "
              << materialColours.size() << " materials\n";

    // Only worth caching big models (skip small ones like cube.obj)
    if (V.size() > 100000)
        WriteCache(cachePath, interleaved, indices);

    uploadInterleaved_(interleaved, indices);
    return true;
}

// ----------------------------
// Render
// ----------------------------
void Mesh::RenderMesh() const 
{
    if (VAO == 0 || indexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ----------------------------
// Clear GL resources
// ----------------------------
void Mesh::ClearMesh() 
{
    if (IBO != 0) 
    {
        glDeleteBuffers(1, &IBO);
        IBO = 0;
    }
    if (VBO != 0) 
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0) 
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    indexCount = 0;
}
