#ifndef _BASE_THREAD_
#define _BASE_THREAD_

// WINTHREAD or PTHREAD, Allow using pthread on windows
#if defined(WIN32) && !defined(PTHREAD)
#define WINTHREAD
#else
#define PTHREAD
#endif


#ifdef WINTHREAD
#define WINTHREAD_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#ifdef _MSC_VER
#pragma warning ( disable : 4996 )
#endif
#endif

#ifdef PTHREAD
#include <pthread.h>
#include <unistd.h>
#endif

#include <cstdio>

namespace base {
	class Thread {
		public:
		Thread() : m_running(false), m_priority(0), m_thread(0) {};
		~Thread() { 
			if(m_running) printf("Warning: Thread still running\n");
		}

		/** Begin a new thread
		 * @param cls  Parent class
		 * @param func Function pointer to class member funtion
		 * @param arg  Function argument */
		template<typename T, typename P>
		bool begin(T* cls, void(T::*func)(P), P arg) {
			if(m_running) return false;
			ThreadDataMA<T,P>* data = new ThreadDataMA<T,P>;
			data->cls = cls;
			data->arg = arg;
			data->func = func;
			return _beginThread(data);
		}

		/** Begin a new thread
		 * @param cls  Parent class
		 * @param func Function pointer to class member funtion */
		template<typename T>
		bool begin(T* cls, void(T::*func)()) {
			if(m_running) return false;
			ThreadDataM<T>* data = new ThreadDataM<T>;
			data->cls = cls;
			data->func = func;
			return _beginThread(data);
		}

		/** Begin a new thread
		 * @param func Function pointer to static function
		 * @param arg  Function argument */
		template<typename T>
		bool begin(void(*func)(T), T arg) {
			if(m_running) return false;
			ThreadDataSA<T>* data = new ThreadDataSA<T>;
			data->arg = arg;
			data->func = func;
			return _beginThread(data);
		}

		/** Begin a new thread
		 * @param func Function pointer to static function */
		bool begin(void(*func)()) {
			if(m_running) return false;
			ThreadDataS* data = new ThreadDataS;
			data->func = func;
			return _beginThread(data);
		}

		/** Lambda version? */
		template<class T, class A>
		bool begin(const T& lambda, const A& arg) {
			if(m_running) return false;
			ThreadDataLambda<T,A>* data = new ThreadDataLambda<T,A>(lambda, arg);
			return _beginThread(data);
		}


		// Other functions
		

		/** Terminate thread (not implemented) */
		void terminate();

		/** Is th thread running */
		bool running() const { return m_running; }

		/** Wait here until thread exits */
		void join() const { while(m_running) sleep(10); }
		
		/** set thread priority (WINTHREAD only) */
		void priority(int p) {
			m_priority = p;
			#ifdef WINTHREAD
			if(m_running) SetThreadPriority(m_thread, p);
			#endif
		}

		void setName(const char* name) {
			if(!m_running) return;
			#ifdef PTHREAD
		//	pthread_setname_np(m_thread, name);
			#endif
		}

		/** Tell current thread to sleep (milliseconds) */
		static void sleep(int time) {
			#ifdef PTHREAD
			usleep(time);
			#endif
			#ifdef WINTHREAD
			Sleep(time);
			#endif
		}


		private:
		struct ThreadData {
			Thread* thread;
			virtual void run() = 0;
			virtual ~ThreadData() {}
		};
		struct ThreadDataS : public ThreadData {
			void(*func)();
			void run() { func(); };
		};
		template<typename T> struct ThreadDataSA : public ThreadData {
			T arg;
			void(*func)(T);
			void run() { func(arg); };
		};
		template<typename T> struct ThreadDataM : public ThreadData {
			T* cls;
			void(T::*func)();
			void run() { (cls->*func)(); };
		};
		template<typename T, typename A> struct ThreadDataMA : public ThreadData {
			T* cls;
			A  arg;
			void(T::*func)(A);
			void run() { (cls->*func)(arg); };
		};
		template<typename F, typename A> struct ThreadDataLambda : public ThreadData {
			ThreadDataLambda(const F& func, A arg) : func(func), arg(arg) {}
			void run() { func(arg); };
			F func;
			A  arg;
		};

		bool _beginThread(ThreadData* data) {
			data->thread = this;
			#ifdef WINTHREAD
			m_thread = (HANDLE)_beginthreadex(0, 0, _threadFunc, data, 0, &m_threadID);
			if(m_priority) SetThreadPriority(m_thread, m_priority); //set thread priority
			#endif
			#ifdef PTHREAD
			pthread_attr_init(&pattr);
			pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
			pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
			//usleep(1000);
			pthread_create(&m_thread, &pattr, _threadFunc, data);
			#endif
			
			//thread creation failed
			if(m_thread==0) {
				printf("Failed to create thread\n");
				delete data;
				return false;
			}
			return true;
		}

		#ifdef WINTHREAD
		static unsigned int __stdcall _threadFunc(void* data) {
			ThreadData* d = static_cast<ThreadData*>(data);
			d->thread->m_running = true;
			d->run();
			d->thread->m_running = false;
			delete d;
			return 0;
		}
		#else
		static void* _threadFunc(void* data) {
			ThreadData* d = static_cast<ThreadData*>(data);
			d->thread->m_running = true;
			d->run();
			d->thread->m_running = false;
			pthread_exit(0);
			delete d;
			return 0;
		}
		#endif


		private:
		bool m_running;			//thread status
		int m_priority;			//thread priority
		
		#ifdef WINTHREAD
		HANDLE m_thread;		//Thread handle
		unsigned int m_threadID;//thread ID
		#else
		pthread_t m_thread;		//linux thread
		pthread_attr_t pattr;	//thread attributes
		#endif
	};

	#ifdef WINTHREAD
	class Mutex {
		public:
		Mutex()       { InitializeCriticalSection(&m_lock); }
		~Mutex()      { DeleteCriticalSection(&m_lock); }
		void lock()   { EnterCriticalSection(&m_lock); }
		void unlock() { LeaveCriticalSection(&m_lock); }
		bool tryLock(){ return TryEnterCriticalSection(&m_lock); }
		private:
		CRITICAL_SECTION m_lock;
	};
	#else 
	class Mutex {
		public:
		Mutex()       { pthread_mutex_init(&m_lock, 0); }
		~Mutex()      { pthread_mutex_destroy(&m_lock); }
		void lock()   { pthread_mutex_lock(&m_lock); }
		void unlock() { pthread_mutex_unlock(&m_lock); }
		bool tryLock(){ return pthread_mutex_lock(&m_lock)==0; }
		private:
		pthread_mutex_t m_lock;
	};
	#endif
	
	/** Exception safe mutex aquistion class */
	class MutexLock {
		public:
		MutexLock(Mutex& mutex, bool block=true) : m_mutex(mutex), m_locked(true) {
			if(block) mutex.lock();
			else m_locked = mutex.tryLock();
		}
		~MutexLock() {
			if(m_locked) m_mutex.unlock();
		}
		operator bool() const { return m_locked; }
		protected:
		Mutex& m_mutex;
		bool m_locked;
	};


	/** Thread Barrier - See OgreBarrier.h */
	#ifdef WINTHREAD
	class Barrier {
		public:
		Barrier() { m_semaphores[0] = m_semaphores[1] = NULL; }
		void initialise(int threads) {
			m_count = threads;
			m_semaphores[0] = CreateSemaphore(NULL, 0, threads, NULL);
			m_semaphores[1] = CreateSemaphore(NULL, 0, threads, NULL);
		}
		~Barrier() {
			if(m_count > 0) {
				CloseHandle(m_semaphores[0]);
				CloseHandle(m_semaphores[1]);
			}
		}
		void sync() {
			volatile size_t index = m_index;
			MemoryBarrier();
			LONG old = _InterlockedExchangeAdd(&m_lockCount, 1);
			if(old != (long)m_count-1) WaitForSingleObject(m_semaphores[index], INFINITE);
			else {
				m_index = !index;
				m_lockCount = 0;
				if(m_count>1) ReleaseSemaphore(m_semaphores[index], m_count-1, NULL);
			}
		}
		private:
		size_t m_count = 0;
		size_t m_index = 0;
		HANDLE m_semaphores[2] { NULL, NULL };
		volatile LONG m_lockCount;
	};
	#else
	class Barrier {
		public:
		void initialise(int threads) { pthread_barrier_init(&m_barrier, 0, threads); m_initialised=true; }
		~Barrier() { if(m_initialised) pthread_barrier_destroy(&m_barrier); }
		void sync() { pthread_barrier_wait(&m_barrier); }
		private:
		pthread_barrier_t m_barrier;
		int m_initialised = false;
	};
	#endif

};

#endif
