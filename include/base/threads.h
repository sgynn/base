#ifndef _BASE_THREAD_
#define _BASE_THREAD_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#ifdef _MSC_VER
#pragma warning ( disable : 4996 )
#endif
#endif

#ifdef LINUX
#include <pthread.h>
#endif

#include <iostream>

namespace base {
	class Thread {
		public:
		Thread() : m_running(false), m_priority(0), m_thread(0) {};
		~Thread() { 
			if(m_running) std::cout << "Warning: Thread still running\n";
			// wait for thread to end
			// close thread handle
		};
		
		/** start a new thread
		 * @param func Function for the thread to run
		 * @param arg Argument to pass to the thread
		 */
		template <typename T>
		bool begin(void(func)(T), T arg) {
			if(m_running) return false;
			ThreadArg<T> *a = new ThreadArg<T>;
			a->thread = this;
			a->arg = arg;
			a->func = func;
			#ifdef WIN32
			m_thread = (HANDLE)_beginthreadex(0, 0, threadFunc<T>, a, 0, &m_threadID);
			if(m_priority) SetThreadPriority(m_thread, m_priority); //set thread priority
			#endif
			#ifdef LINUX
			pthread_attr_init(&pattr);
			pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
			pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
			//usleep(1000);
			pthread_create(&m_thread, &pattr, threadFunc<T>, a);
			#endif
			
			//thread creation failed
			if(m_thread==0) {
				std::cout << "Failed to create thread\n";
				delete a;
				return false;
			}
			return true;
		}
		/// Terminate thread
		void terminate();
		/// is the thread running?
		bool running() const { return m_running; }
		
		/// set thread priority
		void priority(int p) {
			m_priority = p;
			#ifdef WIN32
			if(m_running) SetThreadPriority(m_thread, p); //set priority now
			#endif
		}
		
		private:
		bool m_running;			//thread status
		int m_priority;			//thread priority
		
		#ifdef WIN32
		HANDLE m_thread;		//Thread handle
		unsigned int m_threadID;	//thread ID ?
		#else
		pthread_t m_thread;		//linux thread
		pthread_attr_t pattr;		//thread attributes
		#endif
		
		//the arguments to sent to the thread
		template<typename T>
		struct ThreadArg {
			T arg;
			Thread* thread;
			void(*func)(T);
		};
		
		// Thread function wrapper to make it look nicer with all the templatey stuff
		template<typename T>
		#ifdef WIN32
		static unsigned int __stdcall threadFunc(void* arg) {
		#else
		static void* threadFunc(void* arg) {
		#endif
			ThreadArg<T> *a = (ThreadArg<T>*)arg;
			a->thread->m_running = true;
			a->func(a->arg);
			a->thread->m_running = false;
			delete a;

			#ifdef LINUX
			pthread_exit(0);
			#endif
			return 0;
		}
	};

	#ifdef WIN32
	class Mutex {
		public:
		Mutex()       { InitializeCriticalSection(&m_lock); }
		~Mutex()      { DeleteCriticalSection(&m_lock); }
		void lock()   { EnterCriticalSection(&m_lock); }
		void unlock() { LeaveCriticalSection(&m_lock); }
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
		private:
		pthread_mutex_t m_lock;
	};
	#endif
	

};

#endif
