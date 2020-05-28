#pragma once

#include "array.h"
#include "hash.h"

template <typename T>
class hash_array{
	public:
		
	
	private:
		array<bool> is_valid_;	//also stores length (should always be pow(2, n)
		T *data_ = nullptr;
		
		enum slot_type : bool{
			EMPTY = false,
			FILLED = true
		};
		
		template <slot_type type = FILLED>
		int find(const T &key)
		{
			hash_type hash_value = get_hash(key); 
			//consider hash shuffle
			int score_rehash = 0;
			for(int i = 0; true; i++){
				int hash_index = (hashv + i) & (is_valid_.size()-1);
				if(is_valid_[hash_index]){
					score_rehash++;
					if(type == FILLED && data_[hash_index] == key){
						return hash_index;
					}
				}
				if(!is_valid_[hash_index]){
					if constexpr(type == EMPTY){
						//if we are looking for an empty value
						//rehash if we had to search more than 1/8th of the array
						if(score_rehash > is_valid_.size() >> 3){
							return -1;
						}
						return hash_index;
					}
				}
				
				if(i == is_valid_.size()){ 
					return -1;
				}
			}
		}
};
