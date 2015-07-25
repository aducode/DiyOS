#ifndef _DIYOS_MAP_H
#define _DIYOS_MAP_H

/**
 * 计算key hash值
 */
typedef int (* cal_hash_code_ptr) (void *);
/**
 * 比较key的值
 */
typedef int (* cmp_ptr)(void *, void *);

/**
 * @struct entry
 * @brief map的entry
 * @field key 
 * @field value
 */
struct entry {
	void * key;
	void * value;
};


/**
 * @struct map
 * @brief hashmap
 * @param size;
 * @param capacity;
 * @param entry;
 * @param cal_hash_code;
 */
struct map {
		int size;							//entry数量
		int capacity;						//总容量
		struct entry * entries;				//连续内存
		cal_hash_code_ptr cal_hash_code; 	//计算keyhash值的函数
		cmp_ptr cmp;						//比较key的值
};

/**
 * @function get
 * @brief 根据key获取value
 * @param map
 * @param key
 * @return  value
 */
extern void * get(sturct map * map, void * key);

/**
 * @function set
 * @brief 设置
 * @param map
 * @param key
 * @param value
 * @return 0 success
 */
extern int set(struct map * map, void * key, void * value);

/**
 * @function delete
 * @brief 删除
 * @param map
 * @param key
 * @return 0 success
 */
extern int delete(struct map * map, void * key);
#endif
