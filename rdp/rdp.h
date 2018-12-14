/**
 * RDP 可靠传输层
 */
/** 传输可靠度 */
enum
{
    RDP_UNRELIABLE      = 0,
    RDP_UNRELIABLE_SEQ  = 1,
    RDP_RELIABLE        = 2,
    RDP_RELIABLE_SEQ    = 2 | 1,
    RDP_RELIABLE_ORDER  = 2 | 1 | 4,
};
/** 传输优先级 */
enum
{
    RDP_PRIORITY_LOW    = 1<<3,
    RDP_PRIORITY_MEDIUM = 2<<3,
    RDP_PRIORITY_HIGH   = 3<<3,
};
typedef struct rdp_t rdp_t;

/**
 * 创建可靠传输环境
 * @param[in] mtuSize  允许的最大发送数据长度
 * @param[in] send 内部使用的发送函数
 * @return 返回可靠传输对象
 */
rdp_t* rdp_context(int mtuSize, int (*send)(const void* data, unsigned len));

/** 关闭传输环境 */
void rdp_shutdown(rdp_t* rdp);

/** 获取统计信息 */
const char* rdp_stats(rdp_t* rdp);

/**
 * 将网络原始数据,放入环境
 */
void rdp_feed(rdp_t* rdp, const void* data, unsigned len);

/**
 * 发送报文
 *
 * @param[in] flags  RDP_PRIORITY_*和RDP_RELIABLE_*的组合
 * @param[in] channel 次序包所在的排序列表号
 */
int rdp_send(rdp_t* rdp, const void *data, unsigned len, unsigned flags, unsigned char channel);

/** 
 * 获取报文
 */
void* rdp_recv(rdp_t* rdp, int timeoutMs, unsigned* len);

/**
 * 释放报文
 */
void rdp_free(void* packet);

