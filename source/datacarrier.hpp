#ifndef DATACARRIER_HPP
#define DATACARRIER_HPP

#include <string>
#include <sstream>

template<typename T>
struct Pos
{
    Pos() {}
    Pos(T x, T y, T z) :
        x(x), y(y), z(z) {}

    bool isNeighbour(const Pos &pos, T range) const {
        return (x - pos.x) * (x - pos.x) + (y - pos.y) * (y - pos.y) + (z - pos.z) * (z - pos.z) < range * range;
    }

    bool operator==(const Pos &rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    Pos &operator+=(const T &value) {
        x += value;
        y += value;
        z += value;
        return *this;
    }
    Pos &operator+=(const Pos &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    Pos &operator-=(const T &value) {
        x -= value;
        y -= value;
        z -= value;
        return *this;
    }
    Pos &operator-=(const Pos &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    Pos operator+(const T &value) const {
        return Pos(x + value, y + value, z + value);
    }
    Pos operator+(const Pos &rhs) const {
        return Pos(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Pos operator-(const T &value) const {
        return Pos(x - value, y - value, z - value);
    }
    Pos operator-(const Pos &rhs) const {
        return Pos(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    T x = T();
    T y = T();
    T z = T();
};
using Posc = Pos<char>;
using Poss = Pos<short>;
using Posi = Pos<int>;
using Posli = Pos<long long>;
using Posf = Pos<float>;
using Poslf = Pos<double>;

namespace std
{

template<typename T>
struct hash<Pos<T>>
{
    size_t operator()(const Pos<T> &pos) {
        size_t h1 = std::hash<T>()(pos.r);
        size_t h2 = std::hash<T>()(pos.g);
        size_t h3 = std::hash<T>()(pos.b);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

}

struct Block
{
    Block() {}
    Block(std::string blockId, Posi pos) :
        blockId(blockId), pos(pos) {}

    bool isNeighbour(const Block &other, int range = 1) const {
        return pos.isNeighbour(other.pos, range);
    }

    std::string blockId;
    Posi pos;
};

struct Particle
{
    Particle() {}
    Particle(std::string particleId, Poslf pos, int durationms = 0) :
        particleId(particleId), pos(pos), durationms(durationms) {}

    std::string particleId;
    Poslf pos;
    int durationms = 0;
};

struct BlockCluster
{
    BlockCluster() {}
    BlockCluster(std::string blockId, Posi posFrom, Posi posTo) :
        blockId(blockId), posFrom(posFrom), posTo(posTo) {}

    std::string blockId;
    Posi posFrom;
    Posi posTo;
};

struct BlockCube
{
    using String1D = std::vector<std::string>;
    using String2D = std::vector<String1D>;
    using String3D = std::vector<String2D>;

    BlockCube(int x, int y, int z) :
        x(x), y(y), z(z), size(x *y *z)
    {
        blockIds = String3D(x, String2D(y, String1D(z)));
    }

    String2D &operator[](int z) {
        return blockIds[z];
    }
    const String2D &operator[](int z) const {
        return blockIds[z];
    }

    String3D blockIds;
    int size = 0;
    int x = 0;
    int y = 0;
    int z = 0;
};

#endif // !DATACARRIER_HPP
