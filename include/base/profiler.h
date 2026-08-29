#pragma once

#include <base/game.h>

namespace base {
	class Profiler {
		public:
		static Profiler& getInstance() { static Profiler inst; return inst; }
		uint allocateID(const char* name) {
			m_names.push_back(name);
			return m_allocatedIDs++;
		}
		void notifyFrameStart() {
			++m_currentFrame &= 3;
			m_stack.clear();
			m_stack.push_back(&m_frame[m_currentFrame]);
			m_stack[0]->children.clear();

			m_data[m_currentFrame&1].resize(m_allocatedIDs);
			for(Data& d: m_data[m_currentFrame&1]) d.min = d.max = d.total = d.count = 0;
		}

		public:
		struct Frame {
			uint id;
			uint64 start, end;
			std::vector<Frame> children;
		};
		struct Data {
			int count;
			uint64 min, max, total;
		};
		const std::vector<Data> getData() const { return m_data[~m_currentFrame&1]; }
		const Frame& getFrame() const { return m_frame[(m_currentFrame-1)&3]; }
		const char* getName(uint id) const { return m_names[id]; }
		
		private:
		friend class BlockProfiler;
		uint64 m_frameStart;
		std::vector<const char*> m_names;
		std::vector<Data> m_data[2];
		std::vector<Frame*> m_stack;
		Frame m_frame[4];
		int m_currentFrame = 0;
		int m_allocatedIDs = 0;
	};

	class BlockProfiler {
		public:
		BlockProfiler(int id) : m_id(id) {
			m_startTime = Game::getTicks();
			Profiler& inst = Profiler::Profiler::getInstance();
			inst.m_stack.back()->children.push_back({m_id, m_startTime, 0});
			inst.m_stack.push_back(&inst.m_stack.back()->children.back());
		}
		~BlockProfiler() {
			Profiler& inst = Profiler::Profiler::getInstance();
			if(m_id >= inst.m_data[inst.m_currentFrame&1].size()) return; // id allocated this frame
			uint64 endTime = Game::getTicks();
			uint64 ticks = endTime - m_startTime;	
			Profiler::Data& data = inst.m_data[inst.m_currentFrame&1][m_id];
			if(data.count==0) data.min = data.max = data.total = ticks;
			else {
				data.min = std::min(data.min, ticks);
				data.max = std::max(data.max, ticks);
				data.total += ticks;
			}
			++data.count;

			inst.m_stack.back()->end = endTime;
			inst.m_stack.pop_back();
		}
		protected:
		uint m_id;
		uint64 m_startTime;
	};
}

#define PROFILE_BLOCK(name) \
	static uint _profilerID = base::Profiler::getInstance().allocateID(#name); \
	base::BlockProfiler _profilerInstance(_profilerID); \
	(void)_profilerInstance; //hide unused variable warnings

#define PROFILE_FRAME \
	base::Profiler::getInstance().notifyFrameStart(); \
	PROFILE_BLOCK(Frame);


