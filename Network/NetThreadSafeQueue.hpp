#pragma once
#include "includes.hpp"
#include "MessageTypes.hpp"

namespace net 
{
	template<typename T>
	class tsQueue 
	{
	public:
		tsQueue() {};
		tsQueue(const tsQueue<T>&) = delete;
		tsQueue(const tsQueue<T>&&) = delete;
		virtual ~tsQueue() { this->Clear(); }

		const T& Front() {
			std::scoped_lock lock(mQueueMut);
			return mQueueDeq.front();
		}

		const T& Back() {
			std::scoped_lock lock(mQueueMut);
			return mQueueDeq.back();
		}

		void PushBack(const T& item) {
			std::scoped_lock lock(mQueueMut);
			mQueueDeq.emplace_back(std::move(item));

			std::unique_lock<std::mutex> uLock(m_NopMut);
			m_Nop.notify_one();
		}

		void PushFront(const T& item) {
			std::scoped_lock lock(mQueueMut);
			mQueueDeq.emplace_front(std::move(item));

			std::unique_lock<std::mutex> uLock(m_NopMut);
			m_Nop.notify_one();
		}

		T PopBack() {
			std::scoped_lock lock(mQueueMut);
			auto front = std::move(mQueueDeq.back());
			mQueueDeq.pop_back();
			return front;
		}

		T PopFront() {
			std::scoped_lock lock(mQueueMut);
			auto front = std::move(mQueueDeq.front());
			mQueueDeq.pop_front();
			return front;
		}

		bool Empty() {
			std::scoped_lock lock(mQueueMut);
			return mQueueDeq.empty();
		}

		size_t Size() {
			std::scoped_lock lock(mQueueMut);
			return mQueueDeq.size();
		}

		void Clear() {
			std::scoped_lock lock(mQueueMut);
			mQueueDeq.clear();
		}

		void Wait() {
			while (Empty()) {
				std::unique_lock<std::mutex> uLock(m_NopMut);
				m_Nop.wait(uLock);
				//std::cout << "woke up\n";
			}
		}

	protected:
		std::mutex mQueueMut;
		std::deque<T> mQueueDeq;

		std::condition_variable m_Nop;
		std::mutex m_NopMut;
	};
}