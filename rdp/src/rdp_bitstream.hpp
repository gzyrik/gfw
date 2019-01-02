#ifndef __RDP_BITSTREAM_HPP__
#define __RDP_BITSTREAM_HPP__
#include "rdp_basictypes.hpp"
#include <string>

namespace rdp {
/** �������͵�λ��С */
#define bitsof(T)       (sizeof(T)*8)
/** ���ض�����ֽڵĸ��� */
inline int bits2bytes(const int bits)
{
    return (bits + 7) >> 3;
}
/** ��ת�ַ��� */
void flipEndian(void* buf, const int size, int count);

/** ��дλ�������� */
struct BitStream
{
    /**
     *����λ������.
     *
     *@param[in] buf ��������Ӧ���ڴ�
     *@param[in] size �ڴ�� buf �ĳ���
     *@param[in] written дָ��λ��
     *@param[in] read ��ָ��λ��
     *@note
     * - ��written < 0, ��дλ���Զ��Ƶ�size
     * - ��read < 0, ���λ���Զ��Ƶ�written
     * ��˵�ǰ�ɶ�[read,written), ��д[written, size)
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
     * д��λ����.
     * ����β���ֽ�ʱ, ͨ���Ҳ�ĵ�λ������Ч����.
     * ���Ĭ��rightAlignedBitsΪtrue,β�����Ҳ��λ��Ϊ��Ч����.
     * ��֮, ������λ��Ϊ��Ч����, ��������Ƚ��ټ�.
     *
     * @param[in] in Դ���ݵ�ַ
     * @param[in] bitLength д���λ����
     * @param[in] rightAlignedBits β���ֽ��Ƿ��Ҳ��λ��Ч
     * @return д��ɹ����� true
     */
    bool write(const void* in, const int bitLength, const bool rightAlignedBits = true);
    /**
     * ��ȡλ����.
     * д��β���ֽ�ʱ, ͨ��Ҫ���뵽�Ҳ��λ,
     * ���alignBitsToRightΪtrueʱ,β���ֽڽ������Ҳ�д��.
     * ��֮, �������,�Ծ�������λд��, ��������Ƚ��ټ�.
     *
     * @param[out] out ��ȡ��λ����
     * @param[in] bitLength ��ȡ��λ����
     * @param[in] alignBitsToRight β���ֽڽ������Ҳ�д��
     * @return д��ɹ����� true
     */
    bool read(void* out, const int bitLength, const bool alignBitsToRight = true);
    /**
     * ѹ��д��λ����.
     * ���ֽ����ĸ�λͨ������0x00,(������0xFF),
     * ���Խ���ѹ����1
     * @param[in] in Դ���ݵ�ַ
     * @param[in] bytesLength д����ֽڳ���
     * @param[in] unsignedData �Ƿ��޷�����ѹ��
     * @return д��ɹ����� true
     */
    bool compressWrite(const void* in, const int bytesLength, const bool unsignedData=true);
    /**
     * ��ȡѹ����λ����
     * @param[out] out ��ȡ��λ����
     * @param[in] bytesLength ��ȡ���ֽڳ���
     * @param[in] unsignedData �Ƿ��޷�����ѹ��
     * @return д��ɹ����� true
     */
    bool compressRead(void* out, const int bytesLength, const bool unsignedData=true);

    /**
     * ���뵽�ֽ�, ��д���������ֽڿ�.
     *
     * @param[in] in Դ���ݵ�ַ
     * @param[in] bitLength д��λ����
     * @param[in]
     * @return д��ɹ����� true
     */
    bool alignedWrite(const void* in,  const int bitLength, const bool rightAlignedBits = true);

    /**
     * ���뵽�ֽ�, ����ȡ�������ֽڿ�.
     *
     * @param[out] out ��ȡ��λ����
     * @param[in] bitLength ��ȡλ����
     * @return д��ɹ����� true
     */
    bool alignedRead(void* out,  const int bitLength, const bool alignBitsToRight = true);

    /** д��1 bit */
    bool write(const bool b)
    {
        uint8_t ch = b ? 0xFF : 0;
        return write(&ch, 1, true);
    }
    /** ��ȡ1 bit */
    bool read(bool& b)
    {
        uint8_t ch;
        if (!read(&ch, 1, true))
            return false;
        b = (ch != 0);
        return true;
    }
    /** д���ַ���*/
    bool write(const std::string& str)
    {
        return compressWrite((uint16_t)str.size()) && write(str.data(), str.size()*8); 
    }
    /** ��ȡ�ַ��� */
    bool read(std::string& str)
    {
        uint16_t len;
        if (!compressRead(&len, sizeof(len)))
            return false;
        str.resize(len);
        return read((void*)str.data(), len*8, true);
    }
    /** д����� */
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
    /** ��ȡ���� */
    template<typename T> bool read(T& obj)
    {
        if (!read(&obj, bitsof(T), true))
            return false;
        if (sizeof(T) > 1 && BitStream::_flipEndian)
            flipEndian(&obj, sizeof(T), 1);
        return true;
    }
    /** ����д��������� */
    template<typename T> bool alignedWrites(const T obj[], const int count)
    {
        return alignedWrite(reinterpret_cast<const void*>(obj), bitsof(T)*count);
    }
    /** �����ȡ�������� */
    template<typename T> bool alignedReads(T obj[], const int count)
    {
        if (!alignedRead(reinterpret_cast<void*>(obj), bitsof(T)*count))
            return false;
        if (sizeof(T) > 1 && BitStream::_flipEndian)
            flipEndian(reinterpret_cast<uint8_t*>(obj), sizeof(T), count);
        return true;
    }
    /** ѹ��д����� */
    template<typename T> bool compressWrite(const T& obj, const bool unsignedData=true)
    {
        return compressWrite(reinterpret_cast<const void*>(&obj), sizeof(T), unsignedData);
    }
    /** ��ȡѹ���Ķ��� */
    template<typename T> bool compressRead(T& obj, const bool unsignedData=true)
    {
        return compressRead(reinterpret_cast<void*>(&obj), sizeof(T), unsignedData);
    }
    /**
     * ������Ӧ�Ĵ�ӡ.
     * 
     * @param[in] scratch �Ƿ��ͷ��ʼ��ӡ,����ӵ�ǰ��λ��
     * @return ���ض�Ӧ�ַ���
     */
    std::string toString(const bool scratch = false) const;

    uint8_t *bitBuffer;///< ���ݵĵ�ַ
    int bitSize;  ///< ���ݵĳ���
    int bitWrite; ///< ��ǰдλ��,��bitsWrite>=bitsSize�򲻿�д
    int bitRead;  ///< ��ǰ��λ��,��bitsRead>=bitsWrite�򲻿ɶ�
    static bool _flipEndian;
};
}
#endif
