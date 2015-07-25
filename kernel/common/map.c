/******************************************************/
/*                 map数据结构		                  */
/******************************************************/
#include "map.h"
static struct entry * find(struct map *map, void *key);


struct entry * find(struct map * map, void *key){
	int hashcode = map->cal_hash_code(key) & (map->capacity - 1);
	int first_hashcode = -1;
	while (1){
		if (first_hashcode >0 && (map->entries + hashcode)->flag == UNUSE){
			//flag==UNUSE表示未使用
			break;
		}
		if ((map->entries + hashcode)->flag == USING && map->cmp((map->entries + hashcode)->key, key) == 0)
		{
			//flag == USING 说明正在被使用
			//找到
			return map->entries + hashcode;
		}
		//hash冲突，找下一个
		if (first_hashcode == -1){
			first_hashcode = hashcode;
		}
		else if (first_hashcode == hashcode){
			//第二次first_hashcode 与 hashcode 相等时，说明已经全部遍历完了
			break;
		}
		hashcode++;
		if (hashcode >= map->capacity){
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
void * get(struct map * map, void * key)
{
	struct entry * entry = find(map, key);
	return entry!=0 ? entry->value : 0;
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
	int hashcode = (map->cal_hash_code(key)) & (map->capacity - 1);
	while ((map->entries + hashcode)->flag == USING && map->cmp((map->entries + hashcode)->key, key)!=0){
		if (first_hashcode == -1){
			first_hashcode = hashcode;
		}
		else if (first_hashcode == hashcode){
			//第二次相等时说明遍历过一遍了
			return -1; //没有空位
		}
		hashcode++;
		if (hashcode >= map->capacity){
			hashcode = 0;
		}
	}
	if ((map->entries + hashcode)->flag != USING){
		(map->entries + hashcode)->key = key;
		map->size++;
	}
	(map->entries + hashcode)->value = value;
	(map->entries + hashcode)->flag = USING;
	return 0;
}


/**
* @function delete
* @brief 删除
* @param map
* @param key
* @return 0 success
*/
int del(struct map * map, void * key)
{
	if (map->size <= 0){
		return -1;
	}
	struct entry * entry = find(map, key);
	if (entry == 0){
		return -1;
	}
	entry->key = 0;
	entry->value = 0;
	entry->flag = USED;
	map->size--;
	return 0;

}