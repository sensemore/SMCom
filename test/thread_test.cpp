#include "thread_test.h"


auto time_start = std::chrono::system_clock::now();

long unsigned tick(){
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time_start).count();
}

template <typename T>
bool sync_queue<T>::receive(T & rec, uint32_t timeout_ms){

	bool ret = false;

	if(timeout_ms == MAX_DELAY){
		if(mutex.try_lock()){
			if(que.size()>0){
				rec = que.front();
				que.pop();
				ret = true;
			}
			else{
				ret = false;
			}
			mutex.unlock();
			return ret;
		}
	}
	else{
		if(mutex.try_lock_for(std::chrono::milliseconds(timeout_ms))){
			if(que.size()>0){
				rec = que.front();
				que.pop();
				ret = true;
			}
			else{
				ret = false;
			}
			mutex.unlock();
			return ret;
		}
		else{
			return false;
		}
	}
	return false;
}

template <typename T>
bool sync_queue<T>::send(const T &msg, uint32_t timeout_ms){
	if(mutex.try_lock_for(std::chrono::milliseconds(timeout_ms))){
		que.push(msg);
		mutex.unlock();
		return true;
	}
	return false;	
}




static sync_queue<std::string> myqueue;

// The function we want to execute on the new thread.
void receiver(int id){
	// std::string received;
	// for(;;){
	// 	if(myqueue.receive(received,MAX_DELAY)){
	// 		printf("(%lu) %d <--- '%s'\n",tick(),id,received.c_str());
	// 	}
	// }
}

// The function we want to execute on the new thread.
void sender(int id){

	// while(1){	
	// 	if(myqueue.send(send_str,MAX_DELAY)){
	// 		printf("(%lu)%d ---> %s \n",tick(),id,send_str.c_str());
	// 	}
	// 	else{
	// 		printf("(%lu)%d --X--> %s\n",tick(),id,send_str.c_str());
	// 	}

	// 	std::this_thread::sleep_for(1000ms);
	// }

}


std::vector<std::thread> thread_test(){
    // Constructs the new thread and runs it. Does not block execution.
    std::vector<std::thread> thread_vector;


	// std::thread trec1(receiver, 0);
	// std::thread trec2(receiver, 1);
	// std::thread trec3(receiver, 2);
	// std::thread trec4(receiver, 3);
	// std::thread tsend1(sender, 13);
	


	// thread_vector.push_back(std::move(trec1));
	// thread_vector.push_back(std::move(trec2));
	// thread_vector.push_back(std::move(trec3));
	// thread_vector.push_back(std::move(trec4));


	// thread_vector.push_back(std::move(tsend1));

    return thread_vector;
}
