#ifndef UVPP_POOL_OBJECT_HEAD
#define UVPP_POOL_OBJECT_HEAD

#include <deque>

const bool ALLOCNEW = true;

namespace uvpp {
	template <typename T, bool A=ALLOCNEW>
	class objpool {
	public:
		~objpool() {
			_mtx.lock();
			while( _pool.size() > 0 ) {
				delete _pool.front();
				_pool.pop_front();
			} 
			_mtx.unlock();
		}

		T* get() {
			if (_pool.size() == 0) {
				if (A) {
					return new T();
				}
				return (T*)malloc(sizeof(T));
			}
			_mtx.lock();
			T* obj = _pool.front();
			_pool.pop_front();
			_mtx.unlock();
			return obj;
		}

		void put(T* obj) {
			_mtx.lock();
			_pool.push_back(obj);
			_mtx.unlock();
		}

		size_t count() {
			return _pool.size();
		}

	private:
		std::deque<T*> _pool;
		uvpp::mutex _mtx;
	};
};

#endif
