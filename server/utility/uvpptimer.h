#ifndef UVPP_TIMER_HEAD
#define UVPP_TIMER_HEAD

namespace uvpp {
	template <typename T>
	class timer: public uvpp::itimer {
	public:
		typedef void (T::*ontime_cb)();

		timer(uint64_t timeout, uint64_t repeat, ontime_cb cb, T* caller) {
			_this = caller;
			_ontime = cb;
			_timer_req.data = this;
			uv_timer_init(uvpp_loop, &_timer_req);
			uv_timer_start(&_timer_req, uvpp_timer_cb, timeout, repeat);
		}

		~timer() {
			uv_timer_stop(&_timer_req);
		}

		void attach(ontime_cb cb, T* caller) {
			_this = caller;
			_ontime = cb;
		}

		virtual void ontimer() {
			if (_this) (_this->*_ontime)();
		}

	private:
		T* _this;
		ontime_cb _ontime;
		uv_timer_t _timer_req;
	};
};

#endif
