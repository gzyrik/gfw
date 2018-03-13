#ifndef __X_HASH_H__
#define __X_HASH_H__
#include "xtype.h"
X_BEGIN_DECLS

#define     x_hash_table_insert(hash_table, key, value)                 \
    x_hash_table_insert_full (hash_table, key, value, FALSE, FALSE)

#define     x_hash_table_add(hash_table, key)                           \
    x_hash_table_insert_full (hash_table, key, key, TRUE, FALSE)

#define     x_hash_table_remove(hash_table, key)                        \
    x_hash_table_remove_full (hash_table, key, TRUE)

#define     x_hash_table_steal(hash_table, key)                         \
    x_hash_table_remove_full (hash_table, key, FALSE)

#define     x_hash_table_foreach_remove(hash_table, func, user_data)    \
    x_hash_table_foreach_remove_full (hash_table, func, user_data, TRUE)

#define     x_hash_table_foreach_steal(hash_table, func, user_data)     \
    x_hash_table_foreach_remove_full (hash_table, func, user_data, FALSE)

xptr        x_hash_table_lookup     (xHashTable     *hash_table,
                                     xcptr          key);

/**
 * Create a hash table with full arguments.
 *
 * @param[in] hash_func The callback to calculate a hash value from a key
 * @param[in] cmp_func  The callback to compare two keys
 * @param[in] key_destroy the callback to free a key
 * @param[in] val_destroy  the callback to free a value
 */
xHashTable* x_hash_table_new_full   (xHashFunc      hash_func,
                                     xCmpFunc       cmp_func,
                                     xFreeFunc      key_destroy,
                                     xFreeFunc      val_destroy);

/**
 * @brief create a hash table with full arguments.
 * 哈希名和哈希值无须释放
 * @param[in] hash 哈希函数
 * @param[in] cmp 比较函数
 */
#define     x_hash_table_new(hash, cmp)                                 \
    x_hash_table_new_full ((xHashFunc)hash, (xCmpFunc)cmp, NULL, NULL)

/** 增加哈希表的引用计数 */
xHashTable* x_hash_table_ref        (xHashTable     *hash_table);

/** 减少哈希表的引用计数 */
xint        x_hash_table_unref      (xHashTable     *hash_table);

/** 返回当前哈希项数 */
xsize       x_hash_table_size       (xHashTable     *hash_table);    

void        x_hash_table_insert_full(xHashTable     *hash_table,
                                     xptr           key,
                                     xptr           value,
                                     xbool          keep_new_key,
                                     xbool          reusing_key);

xbool       x_hash_table_remove_full(xHashTable     *hash_table,
                                     xptr           key,
                                     xbool          notify);

/** for each item in xHashTable, call
 * if(func(key, val, user_data)) delete item
 */
xuint       x_hash_table_foreach_remove_full    (xHashTable     *hash_table,
                                                 xWalkFunc      func,
                                                 xptr           user_data,
                                                 xbool          notify);

/** for each key in xHashTable, call
 * if(!func(key, user_data)) break
 */
void        x_hash_table_walk_key   (xHashTable     *hash_table,
                                     xWalkFunc      func,
                                     xptr           user_data);

void        x_hash_table_foreach_key(xHashTable     *hash_table,
                                     xVisitFunc     func,
                                     xptr           user_data);

/** for each val in xHashTable, call
 * if(!func(key, user_data)) break
 */
void        x_hash_table_walk_val   (xHashTable     *hash_table,
                                     xWalkFunc      func,
                                     xptr           user_data);

void        x_hash_table_foreach_val(xHashTable     *hash_table,
                                     xVisitFunc     func,
                                     xptr           user_data);

/** for each item in xHashTable, call
 * if(!func(key, val, user_data)) break
 */
void        x_hash_table_walk       (xHashTable     *hash_table,
                                     xWalkFunc      func,
                                     xptr           user_data);

void        x_hash_table_foreach    (xHashTable     *hash_table,
                                     xVisitFunc     func,
                                     xptr           user_data);

xint        x_direct_hash           (xptr           key);
xint        x_direct_cmp            (xptr           key1,
                                     xptr           key2,
                                     ...);
xint        x_str_hash              (xptr           key);
#define     x_str_cmp               x_strcmp

X_END_DECLS
#endif /* __X_HASH_H__ */
