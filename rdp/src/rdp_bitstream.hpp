#ifndef __RDP_BITSTREAM_HPP__
#define __RDP_BITSTREAM_HPP__
#include "rdp_basictypes.hpp"
#include <string>

namespace rdp {
/** 计算类型的位大小 */
#define bitsof(T)       (sizeof(T)*8)
/** 返回对齐的字节的个数 */
inline int bits2bytes(const int bits)
{
    return (bits + 7) >> 3;
}
/** 反转字符序 */
void flipEndian(void* buf, const int size, int count);

/** 读写位的数据流 */
struct BitStream
{
    /**
     *构造位流对象.
     *
     *@param[in] buf 数据流对应的内存
     *@param[in] size 内存块 buf 的长度
     *@param[in] written 写指针位置
     *@param[in] read 读指针位置
     *@note
     * - 若written < 0, 则写位置自动移到size
     * - 若read < 0, 则读位置自动移到written
     * 因此当前可读[read,written), 可写[written, size)
     */
    BitStream(void* buf, const int size, const int written = 0, const int read = 0):
        bitBuffer((uint8_t*)buf),
        bitSize(size),
        bitWrite(written),
        bitRead(read)
    {
        if (bitWrite < 0)
            this->bitWrite = this->bitSize;
        if (bitRead < 0)
            this->bitRead = this->bitWrite;
    }

    /**
     * 写入位数据.
     * 处理尾部字节时, 通常右侧的低位才是有效数据.
     * 因此默认rightAlignedBits为true,尾部将右侧低位作为有效数据.
     * 反之, 将左侧高位作为有效数据, 这种情况比较少见.
     *
     * @param[in] in 源数据地址
     * @param[in] bitLength 写入的位长度
     * @param[in] rightAlignedBits 尾部字节是否右侧低位有效
     * @return 写入成功返回 true
     */
    bool write(const void* in, const int bitLength, const bool rightAlignedBits = true);
    /**
     * 读取位数据.
     * 写入尾部字节时, 通常要对齐到右侧低位,
     * 因此alignBitsToRight为true时,尾部字节将对齐右侧写入.
     * 反之, 无需对齐,仍旧由左侧高位写入, 这种情况比较少见.
     *
     * @param[out] out 读取的位数据
     * @param[in] bitLength 读取的位长度
     * @param[in] alignBitsToRight 尾部字节将对齐右侧写入
     * @return 写入成功返回 true
     */
    bool read(void* out, const int bitLength, const bool alignBitsToRight = true);
    /**
     * 压缩写入位数据.
     * 多字节数的高位通常都是0x00,(负数是0xFF),
     * 可以将其压缩成1
     * @param[in] in 源数据地址
     * @param[in] bytesLength 写入的字节长度
     * @param[in] unsignedData 是否按无符号数压缩
     * @return 写入成功返回 true
     */
    bool compressWrite(const void* in, const int bytesLength, const bool unsignedData=true);
    /**
     * 读取压缩的位数据
     * @param[out] out 读取的位数据
     * @param[in] bytesLength 读取的字节长度
     * @param[in] unsignedData 是否按无符号数压缩
     * @return 写入成功返回 true
     */
    bool compressRead(void* out, const int bytesLength, const bool unsignedData=true);

    /**
     * 对齐到字节, 并写入连续的字节块.
     *
     * @param[in] in 源数据地址
     * @param[in] bitLength 写入位长度
     * @param[in]
     * @return 写入成功返回 true
     */
    bool alignedWrite(const void* in,  const int bitLength, const bool rightAlignedBits = true);

    /**
     * 对齐到字节, 并读取连续的字节块.
     *
     * @param[out] out 读取的位数据
     * @param[in] bitLength 读取位长度
     * @return 写入成功返回 true
     */
    bool alignedRead(void* out,  const int bitLength, const bool alignBitsToRight = true);

    /** 写入1 bit */
    bool write(const bool b)
    {
        uint8_t ch = b ? 0xFF : 0;
        return write(&ch, 1, true);
    }
    /** 读取1 bit */
    bool read(bool& b)
    {
        uint8_t ch;
        if (!read(&ch, 1, true))
            return false;
        b = (ch != 0);
        return true;
    }
    /** 写入字符串*/
    bool write(const std::string& str)
    {
        return compressWrite((uint16_t)str.size()) && write(str.data(), str.size()*8); 
    }
    /** 读取字符串 */
    bool read(std::string& str)
    {
        uint16_t len;
        if (!compressRead(&len, sizeof(len)))
            return false;
        str.resize(len);
        return read((void*)str.data(), len*8, true);
    }
    /** 写入对象 */
    template<typename T> bool write(const T& obj)
    {
        if (sizeof(T) > 1 && BitStream::_flipEndian) {
            uint8_t tmp[sizeof(T)];
            memcpy(tmp, &obj, sizeof(tmp));
            flipEndian(tmp, sizeof(T), 1);
            return write(tmp, bitsof(T), true);
        }
        return write(&obj, bitsof(T), true);
    }
    /** 读取对象 */
    template<typename T> bool read(T& obj)
    {
        if (!read(&obj, bitsof(T), true))
            return false;
        if (sizeof(T) > 1 && BitStream::_flipEndian)
            flipEndian(&obj, sizeof(T), 1);
        return true;
    }
    /** 对齐写入对象数组 */
    template<typename T> bool alignedWrites(const T obj[], const int count)
    {
        return alignedWrite(reinterpret_cast<const void*>(obj), bitsof(T)*count);
    }
    /** 对齐读取对象数组 */
    template<typename T> bool alignedReads(T obj[], const int count)
    {
        if (!alignedRead(reinterpret_cast<void*>(obj), bitsof(T)*count))
            return false;
        if (sizeof(T) > 1 && BitStream::_flipEndian)
            flipEndian(reinterpret_cast<uint8_t*>(obj), sizeof(T), count);
        return true;
    }
    /** 压缩写入对象 */
    template<typename T> bool compressWrite(const T& obj, const bool unsignedData=true)
    {
        return compressWrite(reinterpret_cast<const void*>(&obj), sizeof(T), unsignedData);
    }
    /** 读取压缩的对象 */
    template<typename T> bool compressRead(T& obj, const bool unsignedData=true)
    {
        return compressRead(reinterpret_cast<void*>(&obj), sizeof(T), unsignedData);
    }
    /**
     * 返回相应的打印.
     * 
     * @param[in] scratch 是否从头开始打印,否则从当前读位置
     * @return 返回对应字符串
     */
    std::string toString(const bool scratch = false) const;

    uint8_t *bitBuffer;///< 数据的地址
    int bitSize;  ///< 数据的长度
    int bitWrite; ///< 当前写位置,若bitsWrite>=bitsSize则不可写
    int bitRead;  ///< 当前读位置,若bitsRead>=bitsWrite则不可读
    static bool _flipEndian;
};
}
#endif
