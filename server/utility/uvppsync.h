#ifndef UVPP_SYNC_HEAD
#define UVPP_SYNC_HEAD

#include "uv.h"
const uint64_t CONDSEC = 1000000000;

namespace uvpp {
	class mutex {
	public:
		mutex() {
			uv_mutex_init(&_mtx);
		}

		~mutex() {
			uv_mutex_destroy(&_mtx);
		}

		void lock() {
			uv_mutex_lock(&_mtx);
		}

		void unlock() {
			uv_mutex_unlock(&_mtx);
		}

	private:
		uv_mutex_t _mtx;
	};


	class semaphore {
	public:
		semaphore(unsigned int value=1) {
			uv_sem_init(&_sem, value);
		}

		~semaphore() {
			uv_sem_destroy(&_sem);
		}

		void wait() {
			uv_sem_wait(&_sem);
		}

		int trywait() {
			return uv_sem_trywait(&_sem);
		}

		void post() {
			uv_sem_post(&_sem);
		}

	private:
		uv_sem_t _sem;
	};


	class condition {
	public:
		condition() {
			uv_cond_init(&_cond);
			uv_mutex_init(&_mtx);
		}

		~condition() {
			uv_mutex_destroy(&_mtx);
			uv_cond_destroy(&_cond);
		}

		int wait(uint64_t timeout=0) {
			if (timeout == 0) {
				uv_cond_wait(&_cond, &_mtx);
				return 0;
			}
			return uv_cond_timedwait(&_cond, &_mtx, timeout);
		}

		void post() {
			uv_cond_signal(&_cond);
		}

		void broadcast() {
			uv_cond_broadcast(&_cond);
		}

	private:
		uv_cond_t _cond;
		uv_mutex_t _mtx;
	};
};

#endif
