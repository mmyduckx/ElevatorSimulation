//CSE691 Fianl Project by Wanxiang Xie
//SU Net ID: wxie15  SUID: 408358088
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <condition_variable>
#include <iomanip>
#include <ctime>
#include <windows.h>

using namespace std;

constexpr auto MAX_FLOOR = 30;
constexpr auto FLOOR_HEIGHT = 4;

class pair_hash {
public:
	inline size_t operator()(const pair<int, int>& p) const {
		return p.first * 100;
	}
};

unordered_set<pair<int, bool>, pair_hash> hashset;
condition_variable cv1, cv2;
mutex m1;
mutex m2;
mutex m_cart;
mutex m_show_cart;
mutex m_button;
mutex m_show_button;
vector<vector<bool>> carts(3, vector<bool>(31, false));
vector<vector<bool>> cart_buttons(3, vector<bool>(31, false));
chrono::duration<double> awt;
double ai_int;

int arrivedTaskCount = 0;
int arrivedTargetCount = 0;
int HC5_pickCount = 0;
bool HC5mode = true;
double hc5time = 0;

chrono::steady_clock::time_point prog_start_time = chrono::steady_clock::now();


// count average waiting time
chrono::duration<double> countAWT() {
	if (arrivedTargetCount == 0) {
		return chrono::duration<double>::zero();
	}
	return awt / arrivedTaskCount;
}

// count average running interval time
double countAI() {
	if (arrivedTargetCount == 0) {
		return 0;
	}
	return ai_int / arrivedTargetCount;
}


class Elevator
{
public:
	Elevator() {};
	Elevator(int i) {
		id = i;

		pickTaskTime.resize(MAX_FLOOR + 1, vector<chrono::steady_clock::time_point>(2, chrono::steady_clock::now()));
		arrivedTime.resize(MAX_FLOOR + 1, vector<chrono::steady_clock::time_point>(2, chrono::steady_clock::now()));

		startTime.resize(MAX_FLOOR + 1, vector<chrono::steady_clock::time_point>(MAX_FLOOR + 1, chrono::steady_clock::now()));
		endTime.resize(MAX_FLOOR + 1, chrono::steady_clock::now());
		//target_flag.resize(MAX_FLOOR + 1, false);
		target_src.resize(MAX_FLOOR + 1);
	};
	void goUp(int target_floor);
	int timeUp(int target_floor);
	void goDown(int target_floor);
	void goUp_wayDown();
	void goDown_wayDown();
	int timeDown(int target_floor);
	void setFloor(int floor);
	void clickButton(int floor);
	void clickButton(vector<int> floors);
	void openAndClose();
	void show();
	void scanner();
	void updateCarts();
	void updateCartButtons();
	chrono::duration<double> AWT(int floor, int direction); // waiting time
	double AI_INT(int floor, int direction); // running interval time


	~Elevator() {};


	//private:
	int id = 0;
	double v = 0;
	double v_max = 2.0; // max of speed
	double acc = 1; // acceleration
	double dec = -1; // deceleration
	int capacity = 10; //maximum capacity of 10 people
	int cur_member = 0;
	int cur_floor = 0; //assume init floor is 0F or Ground 
	vector<bool> button = vector<bool>(MAX_FLOOR + 1, false); // button.size() = 31
	const int openAndCloseTime = 10;
	bool runStatus = false;
	bool direction = true; // up is true , down is false
	vector<vector<chrono::steady_clock::time_point> > pickTaskTime;
	vector<vector<chrono::steady_clock::time_point> > arrivedTime;

	vector<vector<chrono::steady_clock::time_point> > startTime;
	vector<chrono::steady_clock::time_point> endTime;
	vector<unordered_set<int>> target_src;
	vector<bool> wt_flag = vector<bool>(MAX_FLOOR + 1, true);

};


void initCarts() {
	unique_lock<mutex> lock(m_cart);

	for (int i = 0; i < 3; ++i) {
		carts[i][0] = true;  //let cart start show at 0F
	}
}

void showPrompt()
{
	auto prog_now_time = chrono::steady_clock::now();
	chrono::duration<double> duration = prog_now_time - prog_start_time;
	double showTime = duration.count() * 10;

	if (HC5mode == false) {
		cout << "Mode: Normal" << endl;
		cout << "Run time: " << showTime << "s" << endl;
		cout << "Responsed Task Count: " << arrivedTaskCount << " ";
		cout << "Arrived Target Count: " << arrivedTargetCount << endl;
		cout << "AWT(average waiting time): " << countAWT().count() * 10 << "s " << endl;
		cout << "AI_INT(average running interval time): " << countAI() * 10 << "s " << endl;
		cout << "Programming is running, You can stop it at any time and check log.txt file." << endl;

	}
	else {
		cout << "Mode: HC5 Test" << endl;
		cout << "Run time: " << showTime << "s" << endl;
		cout << "Responsed Task Count: " << arrivedTaskCount << " ";
		cout << "Arrived Target Count: " << arrivedTargetCount << endl;
		cout << "AWT(average waiting time): " << countAWT().count() * 10 << "s " << endl;
		cout << "AI_INT(average running interval time): " << countAI() * 10 << "s " << endl;
		double HC5 = HC5_pickCount / 500.0;
		cout << "HC5(five minute handling capacity): " << HC5 * 100 << "% " << endl;
		if (hc5time < 500) {
			cout << "Programming is running, Please wait for completion..." << endl;
		}
		else {
			cout << "HC5 time is up, You can stop the programming and check test_HC_log.txt file." << endl;
		}
	}
}


void showCartButtons() {
	unique_lock<mutex> lock(m_show_button);
	cout << "================================================ Button Panel ================================================" << endl;
	cout << "[*]: current [A]: selected []: unselected" << endl;
	cout << "-------------------------------------------------------------------------------------------------------------------------------" << endl;
	cout << "No.";
	for (int i = 0; i <= MAX_FLOOR; ++i) {
		cout << setw(3) << i << "F";
	}
	cout << endl;
	for (int j = 0; j < 3; ++j) {
		cout << j + 1 << "  ";
		for (int i = 0; i <= MAX_FLOOR; ++i) {
			if (carts[j][i] == true)
				cout << setw(4) << "*";
			else if (cart_buttons[j][i] == true)
				cout << setw(4) << "A";
			else if (cart_buttons[j][i] == false)
				cout << setw(4) << " ";
		}
		cout << endl;
	}
	cout << endl;
	cout << "-------------------------------------------------------------------------------------------------------------------------------" << endl;


}

void showCarts() {

	unique_lock<mutex> lock(m_show_cart);
	system("cls");
	cout << "============================================== Building ==============================================" << endl;
	vector<int> cur_floors(3, 0);
	cout << "Cart No.   1  2  3" << endl;
	for (int i = 0; i <= MAX_FLOOR; ++i) {
		cout << setiosflags(ios::right);
		int show_floor = MAX_FLOOR - i;
		cout << setw(2) << MAX_FLOOR - i << "F        ";
		if (carts[0][show_floor] == true) {
			cout << "*  ";
			cur_floors[0] = show_floor;
		}
		else
			cout << "   ";
		if (carts[1][show_floor] == true) {
			cout << "*  ";
			cur_floors[1] = show_floor;

		}
		else
			cout << "   ";
		if (carts[2][show_floor] == true) {
			cout << "*  ";
			cur_floors[2] = show_floor;
		}
		else
			cout << "   ";
		cout << endl;
	}

	cout << "Current F:" << setw(2) << cur_floors[0] << " " << setw(2) << cur_floors[1] << " " << setw(2) << cur_floors[2] << endl;

	showCartButtons();
	showPrompt();
}

void Elevator::setFloor(int floor)
{
	unique_lock<mutex> lock(m2);
	button[floor] = false; //reset current floor button
	auto seed = time(0);
	srand(seed);

	ofstream f_out("log.txt", ios::app);
	arrivedTime[floor][direction] = chrono::steady_clock::now();
	++arrivedTaskCount;
	awt += AWT(floor, direction);

	//pick up passenger and set target floor
	int target_floor = floor;
	//choose mode to set floor 
	if (HC5mode == false) {
		if (floor != MAX_FLOOR && floor != 0) {
			if (direction == true) {
				target_floor = rand() % (MAX_FLOOR + 1 - floor) + floor;
			}
			else {
				target_floor = rand() % floor;
			}
		}
		//set new target
		if (target_floor != floor) {
			if (button[target_floor] == false) {
				button[target_floor] = true;
				pickTaskTime[target_floor][direction] = chrono::steady_clock::now();
				wt_flag[target_floor] = false;
				f_out << "Cart:" << id << " -> Set task (Floor:" << target_floor << " Direction:" << direction << " runStatus:" << runStatus << " algorithm: Waydown)" << endl;
			}

			startTime[floor][target_floor] = chrono::steady_clock::now();

			//source floor isn't exist, add it
			if (target_src[target_floor].find(floor) == target_src[target_floor].end()) {
				target_src[target_floor].insert(floor);
			}

		}

	}
	else {
		if (hc5time >= 500) {
			return;
		}

		//set target floor
		if (floor != MAX_FLOOR && floor != 0) {
			if (direction == true) {
				target_floor = rand() % (MAX_FLOOR + 1 - floor) + floor;
			}
			else {

				target_floor = rand() % (MAX_FLOOR - 1) + 2;
			}
		}

		//set new target (only direction is up)
		bool onedirection = true;
		if (target_floor != floor) {
			if (button[target_floor] == false) {
				HC5_pickCount = HC5_pickCount + 3; //assuming 10 passengers in cart, each time have 10 * 1/3 = 3 choose target floor.

				button[target_floor] = true;
				pickTaskTime[target_floor][onedirection] = chrono::steady_clock::now();
				wt_flag[target_floor] = false;
				f_out << "Cart:" << id << " -> Set task (Floor:" << target_floor << " Direction:" << onedirection << " runStatus:" << runStatus << " algorithm: Waydown)" << endl;
			}

			startTime[floor][target_floor] = chrono::steady_clock::now();

			//source floor isn't exist, add it
			if (target_src[target_floor].find(floor) == target_src[target_floor].end()) {
				target_src[target_floor].insert(floor);
			}

		}

	}

	//count result if current floor(floor) is target
	if (!target_src[floor].empty()) {
		endTime[floor] = chrono::steady_clock::now();

		arrivedTargetCount += target_src[floor].size();
		int target_src_count = target_src[floor].size();
		ai_int += AI_INT(floor, direction);
		double time = AI_INT(floor, direction);
		if (direction == true) {
			for (auto it = target_src[floor].begin(); it != target_src[floor].end();) {
				if (*it < floor) {
					it = target_src[floor].erase(it);
				}
				else {
					it++;
				}
			}
		}
		else
		{
			for (auto it = target_src[floor].begin(); it != target_src[floor].end();) {
				if (*it > floor) {
					it = target_src[floor].erase(it);
				}
				else {
					it++;
				}
			}

		}

		if (target_src_count != 0) {
			double show_int = time / target_src_count;

			f_out << "Cart:" << id << " -> arrived (Floor: " << floor << " Direction: " << direction << ") INT(running interval time):" << show_int * 10 << "s" << endl;
		}

	}


	if (wt_flag[floor] == true) {
		f_out << "Cart:" << id << " -> arrived (Floor: " << floor << " Direction: " << direction << ") WT(wait time):" << AWT(floor, direction).count() * 10 << "s" << endl;
	}
	wt_flag[floor] = true;
	f_out.close();

}

void Elevator::clickButton(int floor)
{

	if (floor >= 0 && floor <= MAX_FLOOR)
		button[floor] = true;

}

void Elevator::clickButton(vector<int> floors)
{
	for (auto i : floors) {
		if (i >= 0 && i <= MAX_FLOOR)
			button[i] = true;
	}


}

int Elevator::timeUp(int target_floor)
{
	if (target_floor < cur_floor) return -1;
	int height = (target_floor - cur_floor) * FLOOR_HEIGHT;
	setFloor(target_floor);
	if (height == 0) return 0;
	else if (height == 1) {
		return 4; //first speedup to v_max and slowdown to 0 taking 4 second
	}
	else {
		int height_vmax = height - 4;
		int t1 = 4;
		int t2 = height_vmax / v_max;
		return t1 + t2;
	}
}

int Elevator::timeDown(int target_floor)
{
	if (target_floor > cur_floor) return -1;
	int height = (cur_floor - target_floor) * FLOOR_HEIGHT;
	setFloor(target_floor);
	if (height == 0) return 0;
	else if (height == 1) {
		return 4; //first speedup to v_max and slowdown to 0 taking 4 second
	}
	else {
		int height_vmax = height - 4;
		int t1 = 4;
		int t2 = height_vmax / v_max;
		return t1 + t2;
	}

}

void Elevator::goUp(int target_floor)
{
	if (target_floor <= cur_floor) return;

	runStatus = true;
	direction = true;
	auto start_time = chrono::steady_clock::now();

	int count = target_floor - cur_floor;
	if (count == 1) {
		this_thread::sleep_for(chrono::milliseconds(1000 * 4) / 10); //4s   shrinking 10 times to shorter waiting time
	}
	else {
		for (int i = 1; i <= count; ++i) {
			if (i == 1 || i == count) {
				this_thread::sleep_for(chrono::milliseconds(1000 * 3) / 10); //3s for first and last floor
			}
			else {
				this_thread::sleep_for(chrono::milliseconds(1000 * 2) / 10); //2s for floor that elevator run max speed
			}
			++cur_floor;
			updateCarts();
			updateCartButtons();
			showCarts();



		}
	}
	auto end_time = chrono::steady_clock::now();
	chrono::duration<double> duration = end_time - start_time;

	ofstream f_out("time.txt", ios::app);
	f_out << "Cart:" << id << " goUp take time -> " << duration.count() * 10 << "s" << endl;
	f_out.close();

}

void Elevator::goDown(int target_floor)
{
	if (target_floor >= cur_floor) return;

	runStatus = true;
	direction = false;
	//cout << "waiting time to go down..." << endl;
	auto start_time = chrono::steady_clock::now();
	//this_thread::sleep_for(chrono::milliseconds(1000 * timeDown(target_floor)) / 100); // shrinking 100 times to shorter waiting time


	int count = cur_floor - target_floor;
	if (count == 1) {
		this_thread::sleep_for(chrono::milliseconds(1000 * 4) / 10); //4s
	}
	else {
		for (int i = 1; i <= count; ++i) {
			if (i == 1 || i == count) {
				this_thread::sleep_for(chrono::milliseconds(1000 * 3) / 10); //3s for first and last floor
			}
			else {
				this_thread::sleep_for(chrono::milliseconds(1000 * 2) / 10); //2s for floor that elevator run max speed
			}
			--cur_floor;
			updateCarts();
			updateCartButtons();
			showCarts();

		}
	}

	//runStatus = false;
	auto end_time = chrono::steady_clock::now();
	chrono::duration<double> duration = end_time - start_time;

	ofstream f_out("time.txt", ios::app);
	f_out << "Cart:" << id << " goDown take time -> " << duration.count() * 10 << "s" << endl;
	f_out.close();
	//cout << "-> " << duration.count() * 10 << "s" << endl; // magnifying 10 times to show
}

void Elevator::goUp_wayDown()
{
	//if no button was pressed, return;
	bool flag = false;
	for (auto i : button) {
		if (i == true) {
			flag = true;
			break;
		}
	}
	if (flag == false) return;



	auto start_time = chrono::steady_clock::now();
	for (int i = 0; i < button.size(); ++i) {
		//arrive each floor and open&close door

		if (button[i] == true && i > cur_floor) {
			goUp(i);  //arrive specfic floor
			setFloor(i); //reset current floor button
			openAndClose(); //open & close door
		}
	}
	auto end_time = chrono::steady_clock::now();
	chrono::duration<double> duration = end_time - start_time;
	ofstream f_out("time.txt", ios::app);
	f_out << "Cart:" << id << " goUp_wayDown take time -> " << duration.count() * 10 << "s" << endl;
	f_out.close();
	runStatus = false;

}

void Elevator::goDown_wayDown()
{
	//if no button was pressed, return;
	bool flag = false;
	for (auto i : button) {
		if (i == true) {
			flag = true;
			break;
		}
	}
	if (flag == false) return;


	auto start_time = chrono::steady_clock::now();
	for (int i = button.size() - 1; i >= 0; --i) {
		//arrive each floor and open&close door

		if (button[i] == true && i < cur_floor) {
			goDown(i);  //arrive specfic floor
			setFloor(i); //reset current floor button
			openAndClose(); //open & close door
		}
	}
	auto end_time = chrono::steady_clock::now();
	chrono::duration<double> duration = end_time - start_time;
	ofstream f_out("time.txt", ios::app);
	f_out << "Cart:" << id << " goDown_wayDown take time -> " << duration.count() * 10 << "s" << endl;
	f_out.close();

	runStatus = false;

}

void Elevator::openAndClose()
{

	//assuming open and close door need 10s
	auto open_time = chrono::steady_clock::now();
	this_thread::sleep_for(chrono::milliseconds(1000 * openAndCloseTime) / 10);
	auto close_time = chrono::steady_clock::now();
	chrono::duration<double> duration = close_time - open_time;

	ofstream f_out("time.txt", ios::app);

	f_out << "Cart:" << id << " Open and Closed ->" << duration.count() * 10 << "s" << endl;
	f_out.close();
}


void Elevator::show()
{
	cout << "==================== Floor Indicator====================" << endl;
	for (int i = 0; i <= MAX_FLOOR; ++i) {
		int show_floor = MAX_FLOOR - cur_floor;
		cout << setiosflags(ios::right);
		cout << setw(2) << MAX_FLOOR - i << " F  ";
		if (i == show_floor)
			cout << "*" << endl;
		else {
			cout << " " << endl;
		}
	}

}

void Elevator::scanner()
{

	int cur = cur_floor;

	for (int i = 1; i <= MAX_FLOOR; ++i) {
		ofstream f_out("log.txt", ios::app);
		if ((cur + i) <= MAX_FLOOR && button[cur + i] == true) { //up
			f_out << "Cart " << id << " Scanner -> result: Up then Down" << endl;
			f_out.close();
			goUp_wayDown();
			goDown_wayDown();
			break;
		}
		else if ((cur - i >= 0) && button[cur - i] == true) //down
		{
			f_out << "Cart " << id << " Scanner -> result: Down then Up" << endl;
			f_out.close();
			goDown_wayDown();
			goUp_wayDown();
			break;
		}
	}


}

void Elevator::updateCarts()
{
	unique_lock<mutex> lock(m_cart);
	for (auto i : carts[id - 1]) {
		i = false;
	}
	carts[id - 1][cur_floor] = true;
}

void Elevator::updateCartButtons()
{
	unique_lock<mutex> lock(m_button);
	for (int i = 0; i < button.size(); ++i) {
		cart_buttons[id - 1][i] = false;
	}

	for (int i = 0; i < button.size(); ++i) {
		cart_buttons[id - 1][i] = button[i];
	}
}


chrono::duration<double> Elevator::AWT(int floor, int direction)
{
	chrono::duration<double> duration = arrivedTime[floor][direction] - pickTaskTime[floor][direction];
	return duration;

}

double Elevator::AI_INT(int floor, int direction)
{
	double dur = 0;
	for (auto i : target_src[floor]) {
		if (direction == true && i < floor) {
			chrono::duration<double> duration = endTime[floor] - startTime[i][floor];
			dur += duration.count();
		}
		else if (direction == false && i > floor) {
			chrono::duration<double> duration = endTime[floor] - startTime[i][floor];
			dur += duration.count();
		}

	}

	return dur;

}



void picker(Elevator& cart1) {
	while (1) {
		unique_lock<mutex> lock1(m1);
		if (HC5mode == false) {
			ofstream f_out("log.txt", ios::app);
			for (auto it = hashset.begin(); it != hashset.end();) {
				//check condition

				if (cart1.runStatus == true) { //waydown algo
					if (cart1.button[it->first] != true && cart1.cur_floor < it->first && cart1.direction == true && it->second == true || cart1.button[it->first] != true && cart1.cur_floor > it->first && cart1.direction == false && it->second == false) {

						//add to cart tasklist
						unique_lock<mutex> lock(m2);
						f_out << "Cart:" << cart1.id << " -> Pick task (Floor:" << it->first << " Direction:" << it->second << " runStatus:" << cart1.runStatus << " algorithm: Waydown)" << endl;
						cart1.clickButton(it->first);
						cart1.pickTaskTime[it->first][it->second] = chrono::steady_clock::now();

						lock.unlock();

						//remove task from hashset
						it = hashset.erase(it);
					}
					else {
						//do nothing...
						//f_out << "do nothing..." << "runStatus:" << cart1.runStatus << endl;
						if (cart1.cur_floor == it->first || cart1.button[it->first] == true) {
							it = hashset.erase(it);
						}
						else {
							it++;
						}

					}
				}
				else { //proximity algo
					if (cart1.button[it->first] != true) {
						unique_lock<mutex> lock(m2);
						f_out << "Cart:" << cart1.id << " -> Pick task (Floor:" << it->first << " Direction:" << it->second << " runStatus:" << cart1.runStatus << " algorithm: Proximity)" << endl;

						//add to cart tasklist
						cart1.clickButton(it->first);
						cart1.pickTaskTime[it->first][it->second] = chrono::steady_clock::now();

						cart1.runStatus = true;
						lock.unlock();
					}
					//remove task from hashset
					it = hashset.erase(it);
				}
			}
			f_out.close();
		}
		else {
			if (hc5time >= 500) break;
			ofstream f_out("log.txt", ios::app);
			for (auto it = hashset.begin(); it != hashset.end();) {
				//check condition
				if (cart1.runStatus == false) { // proximity algo
					if (cart1.button[it->first] != true) {
						unique_lock<mutex> lock(m2);
						f_out << "Cart:" << cart1.id << " -> Pick task (Floor:" << it->first << " Direction:" << it->second << " runStatus:" << cart1.runStatus << " algorithm: Proximity)" << endl;

						//add to cart tasklist
						cart1.clickButton(it->first);
						cart1.pickTaskTime[it->first][it->second] = chrono::steady_clock::now();

						cart1.runStatus = true;
						lock.unlock();
					}
					//remove task from hashset
					it = hashset.erase(it);

				}
				else {  //waydown algo
					if (cart1.button[it->first] != true && cart1.cur_floor < it->first && cart1.direction == true && it->second == true || cart1.button[it->first] != true && cart1.cur_floor > it->first && cart1.direction == false && it->second == false) {

						//add to cart tasklist
						unique_lock<mutex> lock(m2);
						f_out << "Cart:" << cart1.id << " -> Pick task (Floor:" << it->first << " Direction:" << it->second << " runStatus:" << cart1.runStatus << " algorithm: Waydown)" << endl;
						cart1.clickButton(it->first);
						cart1.pickTaskTime[it->first][it->second] = chrono::steady_clock::now();

						lock.unlock();

						//remove task from hashset
						it = hashset.erase(it);
					}
					else {
						//do nothing...
						//f_out << "do nothing..." << "runStatus:" << cart1.runStatus << endl;
						if (cart1.cur_floor == it->first || cart1.button[it->first] == true) {
							it = hashset.erase(it);
						}
						else {
							it++;
						}

					}
				}
			}

			f_out.close();
		}
		lock1.unlock();

		this_thread::sleep_for(500ms);
	}

}

void runElevator(int i) {
	Elevator cart1(i);

	thread pick_thread(picker, ref(cart1));

	//run

	while (1) {
		cart1.scanner();
		//--run;
		this_thread::sleep_for(1s);
	}

	pick_thread.join();

}

void put() {
	prog_start_time = chrono::steady_clock::now();
	if (HC5mode == false) {
		int round = 1;
		while (1) {
			int run = 1;  //put 1 task every round
			auto seed = time(0);
			ofstream f_out("log.txt", ios::app);

			while (run > 0) {
				unique_lock<mutex> lock1(m1);

				srand(seed + run);
				int f = rand() % 30;
				int d = rand() % 2;
				f_out << "Schduler -> receive task (Floor: " << f << " Direction: " << d << " Round:" << round << ")" << endl;
				auto task = make_pair(f, d);
				if (hashset.find(task) == hashset.end()) { //don't exist, insert new one
					hashset.insert(task);
				}
				run--;
				lock1.unlock();

			}

			f_out.close();

			this_thread::sleep_for(5s);
			round++;
		}
	}
	else
	{
		//test HC5(Five Minute Handling Capacity)
		//assume morning high peak has 500 passenger, each cart capicity is 10. So scheduler need to put task at least 500 / 10 = 50 tasks

		int round = 1;
		auto start_time = chrono::steady_clock::now();
		chrono::duration<double> duration = chrono::duration<double>::zero();
		hc5time = duration.count() * 10;
		while (hc5time < 500) {
			int run = 5;
			auto seed = time(0);

			ofstream f_out("test_HC5_log.txt", ios::app);
			while (run > 0) {
				unique_lock<mutex> lock1(m1);
				srand(seed + run);
				int f = rand() % 2; //assuming 0F is basement, 1F is ground, passager have two choices to office 
				bool d = true; //assuming morning high peak only go up
				auto task = make_pair(f, d);
				if (hashset.find(task) == hashset.end()) { //don't exist, insert new one
					hashset.insert(task);
					f_out << "Schduler -> receive task (Floor: " << f << " Direction: " << d << " Round:" << round << ")" << endl;

				}

				run--;
				lock1.unlock();
			}
			f_out.close();

			this_thread::sleep_for(1s);
			round++;

			auto end_time = chrono::steady_clock::now();
			duration = end_time - start_time;
			hc5time = duration.count() * 10;
		}
		ofstream f_out("test_HC5_log.txt", ios::app);
		f_out << "Time is up" << endl;
		f_out << "Total number is 500, HC5 mode pickup count is " << HC5_pickCount << endl;
		double HC5 = HC5_pickCount / 500.0;
		f_out << "HC5 = " << HC5 * 100 << "%" << endl;
		f_out.close();
	}

}



int main() {

	//set console height and wide
	system("mode con cols=150 lines=60");
	int i;
	cout << "==================== Welcome to Elevator Simulation System ====================" << endl;
	cout << "There is two modes: " << endl;
	cout << "1.Normal mode " << endl;
	cout << "2.HC5 Test mode" << endl;
	cout << "Please choose your mode -> ";

	while (1) {
		cin >> i;

		if (i == 1) {
			HC5mode = false;
			break;
		}
		else if (i == 2) {
			HC5mode = true;
			break;
		}
		else {
			cout << "Please choose your mode again." << endl;
		}
	}


	//hide console cursor
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO CursorInfo;
	GetConsoleCursorInfo(handle, &CursorInfo);
	CursorInfo.bVisible = false;
	SetConsoleCursorInfo(handle, &CursorInfo);
	initCarts();

	thread put_thread = thread(put);
	thread cart_thread[3];
	for (int i = 0; i < 3; ++i)
	{
		int id = i + 1;
		cart_thread[i] = thread(runElevator, id);
	}


	put_thread.join();
	for (int i = 0; i < 3; ++i) {
		cart_thread[i].join();
	}


	//cin.get();
	return 0;
}