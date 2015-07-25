/******************************************************/
/*                 map数据结构		                  */
/******************************************************/
#include "map.h"
static struct entry * find(struct map *map, void *key);


struct entry * find(struct map * map, void *key){
	int hashcode = map->cal_hash_code(key) & (map->capacity - 1);
	int first_hashcode = -1;
	while(1){
		if((map->entries+hashcode)->key == 0){
			//key==0表示empty slot
			break;
		} 
		if((map->entries + hashcode)->key!=-1 && map->cmp((map->entries + hashcode)->key, key) == 0)
		{
			//key == -1 说明处于冲突链中间
			//找到
			return map->entries + hashcode;
		}
		//hash冲突，找下一个
		if(first_hashcode == -1){
			first_hashcode = hashcode;
		} else if (first_hashcode == hashcode){
			//第二次first_hashcode 与 hashcode 相等时，说明已经全部遍历完了
			break;
		}
		hashcode++;
		if(hashcode>=capacity){
			hashcode = 0;
		}
	}
	return 0;
}
/**
 * @function get
 * @brief 根据key获取value
 * @param map
 * @param key
 * @return  value
 */
void * get(sturct map * map, void * key)
{
	struct entry * entry = find(map, key);
	return !entry?:entry->value:0;
}

/**
 * @function set
 * @brief 设置
 * @param map
 * @param key
 * @param value
 * @return 0 success
 */
int set(struct map * map, void * key, void * value)
{
	int first_hashcode = -1;
	int hashcode = map->cal_hash_code(key) & (map->capacity - 1);
	while((map->entries + hashcode)->key>0){
		if(first_hashcode==-1){
			first_hashcode = hashcode;
		} else if(first_hashcode == hashcode){
			//第二次相等时说明遍历过一遍了
			return -1; //没有空位
		}
		hashcode++;
		if(hashcode>=map->capacity){
			hashcode=-1;
		}
	}
	(map->entries + hashcode)->key = key;
	(map->entries + hashcode)->value = value;
	return 0
}


/**
 * @function delete
 * @brief 删除
 * @param map
 * @param key
 * @return 0 success
 */
int delete(struct map * map, void * key)
{
	struct entry * entry = find(map, key);
	if(!entry){
		return -1;
	}
	entry->key = -1;
	entry->value = 0;
	return 0;
	
}