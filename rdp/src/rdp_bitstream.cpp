#include "rdp_config.hpp"
#include "rdp_bitstream.hpp"
namespace rdp {
bool BitStream::_flipEndian = false;
void flipEndian(void* buf, const int size, int count)
{
    uint8_t* p = (uint8_t*)buf;
    if (size == 1 || count <= 0)
        return;
    else if(size == 8){
        while(count--) {
            p[0] = p[0] ^ p[7];
            p[7] = p[0] ^ p[7];
            p[0] = p[0] ^ p[7];

            p[1] = p[1] ^ p[6];
            p[6] = p[1] ^ p[6];
            p[1] = p[1] ^ p[6];

            p[2] = p[2] ^ p[5];
            p[5] = p[2] ^ p[5];
            p[2] = p[2] ^ p[5];

            p[3] = p[3] ^ p[4];
            p[4] = p[3] ^ p[4];
            p[3] = p[3] ^ p[4];
            p += 8;
        }
    }
    else if(size == 4) {
        while(count--) {
            p[0] = p[0] ^ p[3];
            p[3] = p[0] ^ p[3];
            p[0] = p[0] ^ p[3];
            p[1] = p[1] ^ p[2];
            p[2] = p[1] ^ p[2];
            p[1] = p[1] ^ p[2];
            p += 4;
        }
    }
    else if(size == 2) {
        while (count--) {
            p[0] = p[0] ^ p[1];
            p[1] = p[0] ^ p[1];
            p[0] = p[0] ^ p[1];
            p += 2;
        }
    }
    else {
        while(count--) {
            int i=0,j=size-1;
            while(i<j) {
                p[i] = p[i] ^ p[j];
                p[j] = p[i] ^ p[j];
                p[i] = p[i] ^ p[j];
                ++i,--j;
            }
            p += size;
        }
    }

}
bool BitStream::write(const void* in, int bitLength, const bool rightAlignedBits)
{
    if (!in || bitLength < 0 || this->bitWrite + bitLength > this->bitSize)
        return false;

    const uint8_t *input=(const uint8_t*)in;
    const int writeOffsetMod8 = this->bitWrite & 7;
    uint8_t* curByte = this->bitBuffer + (this->bitWrite >> 3);
    if (writeOffsetMod8 != 0)//清除写入位置,未对齐的位
        *curByte &= (0xFF << (8 - writeOffsetMod8));

    for (int i=0; bitLength; ++i) {
        uint8_t dataByte = input[i];
        if (bitLength < 8 && rightAlignedBits)   
            dataByte <<= (8 - bitLength);  

        if (writeOffsetMod8 == 0)
            *curByte = dataByte;
        else {
            *curByte |= (dataByte >> writeOffsetMod8); 

            if (bitLength > 8 - writeOffsetMod8)   
                curByte[1] = (uint8_t) (dataByte << (8 - writeOffsetMod8)); 
        }

        if (bitLength >= 8){
            this->bitWrite += 8;
            bitLength -= 8;
            ++curByte;
        }
        else {
            this->bitWrite += bitLength;
            bitLength=0;
        }
    }
    return true;
}
bool BitStream::read(void* out, int bitLength, const bool alignBitsToRight)
{
    if (!out || bitLength < 0 || this->bitRead + bitLength > this->bitWrite)
        return false;

    uint8_t* output = (uint8_t*)out;
    const int readOffsetMod8 = this->bitRead & 7;
    uint8_t* curByte = this->bitBuffer + (this->bitRead >> 3);
    for(int i=0; bitLength; ++i) {
        if (readOffsetMod8 == 0)
            output[i] = *curByte;
        else {
            if (bitLength > 8 - readOffsetMod8)
                output[i] = (curByte[0] << readOffsetMod8) | (curByte[1] >> (8 - readOffsetMod8));
            else
                output[i] = curByte[0] << readOffsetMod8;
        }
        if (bitLength>=8) {
            this->bitRead += 8;
            bitLength -= 8;
            ++curByte;
        }
        else {
            if (alignBitsToRight)
                output[i] >>= (8 - bitLength);
            this->bitRead += bitLength;
            bitLength = 0;
        }		
    }
    return true;
}
bool BitStream::alignedWrite(const void* in,  const int bitLength, const bool rightAlignedBits)
{
    const int bitWrite = (this->bitWrite + 7) & ~7;
    if (bitWrite + bitLength > this->bitSize)
        return false;

    const int bytes = bits2bytes(bitLength);
    uint8_t* dst = this->bitBuffer + (bitWrite>>3);
    memcpy(dst, in, bytes);

    if (bitLength & 7)
        dst[bytes] <<= 8 - (bitLength & 7);

    this->bitWrite = bitWrite + bitLength;
    return true;
}
bool BitStream::alignedRead(void* out,  const int bitLength, const bool alignBitsToRight)
{
    const int bitRead = (this->bitRead+ 7) & ~7;
    if (bitRead+ bitLength > this->bitWrite)
        return false;

    const int bytes = bits2bytes(bitLength);
    const uint8_t* src = this->bitBuffer + (bitRead>>3);
    memcpy(out, src, bytes);

    if (bitLength & 7 && alignBitsToRight) 
        ((uint8_t*)out)[bytes] >>= 8 - (bitLength & 7);

    this->bitRead = bitRead + bitLength;
    return true;
}

bool BitStream::compressWrite(const void* in,  int bytesLength, const bool unsignedData)
{
    if (!in || bytesLength < 0)
        return false;
    const int bitsWrite_ = this->bitWrite;//记录原写位置,用于恢复
    const int bytes_1 = bytesLength - 1;
    const uint8_t byteMatch = unsignedData ? 0 : 0xFF;
    const uint8_t halfMatch = unsignedData ? 0 : 0xF0;
    const uint8_t* input = (const uint8_t*)in;
    int i;
    if (bigEndian()) {
        for(i=0;i<bytes_1; ++i) {
            JIF(write(input[i] == byteMatch));
            if (input[i] != byteMatch){
                JIF(write(input+i, (bytesLength - i) << 3));
                return true;
            }
        }
    }
    else {
        for(i=bytes_1; i; --i) {
            JIF(write(input[i] == byteMatch));
            if (input[i] != byteMatch) {
                JIF(write(input, (i + 1) << 3));
                return true;
            }
        }
    }
    if ((input[i] & 0xF0) == halfMatch){
        JIF(write(true));
        JIF(write(input+i, 4));
    }
    else {
        JIF(write(false));
        JIF(write(input+i, 8));
    }
    return true;
clean:
    this->bitWrite = bitsWrite_;
    return false;
}
bool BitStream::compressRead(void* out, int bytesLength, const bool unsignedData)
{
    if (!out || bytesLength < 0)
        return false;
    const int bitsRead_ = this->bitRead;//记录原写位置,用于恢复
    const int bytes_1 = bytesLength - 1;
    const uint8_t byteMatch = unsignedData ? 0 : 0xFF;
    const uint8_t halfMatch = unsignedData ? 0 : 0xF0;
    uint8_t* output = (uint8_t*)out;
    bool b;
    int i;
    if (bigEndian()) {
        for(i=0;i<bytes_1;++i) {
            JIF(read(b));
            if (b)
                output[i] = byteMatch;
            else {
                JIF(read(output+i, (bytesLength - i) << 3, true));
                return true;
            }
        }
    }
    else {
        for(i=bytes_1; i; --i) {
            JIF(read(b));
            if (b)
                output[i] = byteMatch;
            else {
                JIF(read(output, (i + 1) << 3, true));
                return true;
            }
        }
    }
    JIF(read(b));
    if (b) {
        JIF(read(output+i, 4, true));
        output[i] |= halfMatch;
    }
    else {
        JIF(read(output+i, 8, true));
    }
    return true;
clean:
    this->bitRead = bitsRead_;
    return false;
}
std::string BitStream::toString(bool scratch) const
{
    if (this->bitRead >= this->bitWrite)
        return std::string();

    const uint8_t* curByte = this->bitBuffer + (this->bitRead >> 3);
    int bitsPerByte = 8 - (this->bitRead & 7);//每个字节的有效位数(从高位开始)
    int bitLength = this->bitWrite - this->bitRead;
    std::string ret;
    while (bitLength > 0) {
        //每个字节的最低有效位
        int bitsLow = bitLength >= bitsPerByte ? 0 : bitsPerByte - bitLength;
        for(int i=bitsPerByte-1;i>=bitsLow; --i)
            ret.push_back((*curByte & (1 << i)) ? '1' : '0');
        ++curByte;
        bitLength -= bitsPerByte;
        bitsPerByte = 8;
        if (bitLength > 0)
            ret.push_back(' ');
    }
    return ret;
}
}
