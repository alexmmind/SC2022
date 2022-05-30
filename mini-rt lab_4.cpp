#include <condition_variable>
#include "minirt/minirt.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <queue>
#include <cmath>
#include <mutex>

using namespace minirt;

void initScene(Scene& scene) {
    Color red{ 1, 0.2, 0.2 };
    Color blue{ 0.2, 0.2, 1 };
    Color green{ 0.2, 1, 0.2 };
    Color white{ 0.8, 0.8, 0.8 };
    Color yellow{ 1, 1, 0.2 };

    Material metallicRed{ red, white, 50 };
    Material mirrorBlack{ Color {0.0}, Color {0.9}, 1000 };
    Material matteWhite{ Color {0.7}, Color {0.3}, 1 };
    Material metallicYellow{ yellow, white, 250 };
    Material greenishGreen{ green, 0.5, 0.5 };

    Material transparentGreen{ green, 0.8, 0.2 };
    transparentGreen.makeTransparent(1.0, 1.03);
    Material transparentBlue{ blue, 0.4, 0.6 };
    transparentBlue.makeTransparent(0.9, 0.7);

    scene.addSphere(Sphere{ {0, -2, 7}, 1, transparentBlue });
    scene.addSphere(Sphere{ {-3, 2, 11}, 2, metallicRed });
    scene.addSphere(Sphere{ {0, 2, 8}, 1, mirrorBlack });
    scene.addSphere(Sphere{ {1.5, -0.5, 7}, 1, transparentGreen });
    scene.addSphere(Sphere{ {-2, -1, 6}, 0.7, metallicYellow });
    scene.addSphere(Sphere{ {2.2, 0.5, 9}, 1.2, matteWhite });
    scene.addSphere(Sphere{ {4, -1, 10}, 0.7, metallicRed });

    scene.addLight(PointLight{ {-15, 0, -15}, white });
    scene.addLight(PointLight{ {1, 1, 0}, blue });
    scene.addLight(PointLight{ {0, -10, 6}, red });

    scene.setBackground({ 0.05, 0.05, 0.08 });
    scene.setAmbient({ 0.1, 0.1, 0.1 });
    scene.setRecursionLimit(20);

    scene.setCamera(Camera{ {0, 0, -20}, {0, 0, 0} });
}

struct Point
{
    int x;
    int y;
    Point(int x, int y) : x(x), y(y) {}
};

template<typename T>
class TaskQueue
{
	public:
		condition_variable syncing;
		queue<T> tqueue_;
		mutex locker;

		void PushTask(T value)
		{
			lock_guard<mutex> guard(locker);
			tqueue_.push(value);
			syncing.notify_one();
		}

		T PopTask()
		{
			unique_lock<mutex> lock(locker);
			while (tqueue_.empty())
			{
				syncing.wait(lock);
			}	
			
			T task = tqueue_.front();
			tqueue_.pop();
			return task;
		}
};

class ThreadPool
{
	public:
		vector<thread> threads;
		TaskQueue<Point> task;
		ViewPlane viewPlane;
		int numSamples;
		int numThread;
		Scene scene;
		Image image;
		
		ThreadPool(int numThread, const Scene& scene, const Image& image, ViewPlane& viewPlane, int numSamples): 
			viewPlane(viewPlane)
		{
			this->numSamples = numSamples;
			this->numThread = numThread;
			this->scene = scene;
			this->image = image;
			MakeThreads(numThread);
		};

		void MakeThreads(int numThread)
		{
			for (int i = 0; i < numThread; i++)
			{
				threads.emplace_back([this]() 
				{  
					auto point = task.PopTask();
					while (point.x >= 0)
					{
						const auto color = viewPlane.computePixel(scene, point.x, point.y, numSamples);
						image.set(point.x, point.y, color);
						point = task.PopTask();
					}
				});
			}
		}

		void ThreadsJoin()
		{
			for (int i = 0; i < numThread; i++)
			{
				task.PushTask({-1, -1});
			}
			
			for (int i = 0; i < numThread; i++)
			{
				threads[i].join();
			}
			
			threads.clear();
		}
};

int main(int argc, char** argv) {
    int viewPlaneResolutionX = (argc > 1 ? stoi(argv[1]) : 600);
    int viewPlaneResolutionY = (argc > 2 ? stoi(argv[2]) : 600);
    int numSamples = (argc > 3 ? stoi(argv[3]) : 1);
    string sceneFile = (argc > 4 ? argv[4] : "");

    Scene scene;
    if (sceneFile.empty()) {
        initScene(scene);
    }
    else {
        scene.loadFromFile(sceneFile);
    }

    const double backgroundSizeX = 4;
    const double backgroundSizeY = 4;
    const double backgroundDistance = 15;

    const double viewPlaneDistance = 5;
    const double viewPlaneSizeX = backgroundSizeX * viewPlaneDistance / backgroundDistance;
    const double viewPlaneSizeY = backgroundSizeY * viewPlaneDistance / backgroundDistance;

    ViewPlane viewPlane{ viewPlaneResolutionX, viewPlaneResolutionY,
                         viewPlaneSizeX, viewPlaneSizeY, viewPlaneDistance };

    Image image(viewPlaneResolutionX, viewPlaneResolutionY);

    auto start = chrono::high_resolution_clock::now();
	
	//static 
    //pragma omp parallel for num_threads(1) schedule(static) 
    //#pragma omp parallel for num_threads(2) schedule(static) 
    //#pragma omp parallel for num_threads(4) schedule(static) 
    //#pragma omp parallel for num_threads(8) schedule(static) 
    //#pragma omp parallel for num_threads(16) schedule(static) 
    
    //dynamic
    //#pragma omp parallel for num_threads(1) schedule(dynamic) 
    //#pragma omp parallel for num_threads(2) schedule(dynamic) 
    //#pragma omp parallel for num_threads(4) schedule(dynamic) 
    //#pragma omp parallel for num_threads(8) schedule(dynamic) 
    //#pragma omp parallel for num_threads(16) schedule(dynamic) 
    
    //guided
    //#pragma omp parallel for num_threads(1) schedule(guided) 
    //#pragma omp parallel for num_threads(2) schedule(guided) 
    //#pragma omp parallel for num_threads(4) schedule(guided) 
    //#pragma omp parallel for num_threads(8) schedule(guided) 
    //#pragma omp parallel for num_threads(16) schedule(guided) 
    
    //runtime
    //#pragma omp parallel for num_threads(1) schedule(runtime)
    //#pragma omp parallel for num_threads(2) schedule(runtime)
    //#pragma omp parallel for num_threads(4) schedule(runtime)
    //#pragma omp parallel for num_threads(8) schedule(runtime)
    //#pragma omp parallel for num_threads(16) schedule(runtime)
    
    //auto
    //#pragma omp parallel for num_threads(1) schedule(auto) 
    //#pragma omp parallel for num_threads(2) schedule(auto) 
    //#pragma omp parallel for num_threads(4) schedule(auto) 
    //#pragma omp parallel for num_threads(8) schedule(auto) 
    //#pragma omp parallel for num_threads(16) schedule(auto) 

    auto threadsPool = new ThreadPool(3, scene, image, viewPlane, numSamples);

    for (int x = 0; x < viewPlaneResolutionX; x++)
    for (int y = 0; y < viewPlaneResolutionY; y++){
			threadsPool->task.PushTask({ x, y });
	}
	
    threadsPool->ThreadsJoin();

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> all_time  = end - start;
	
    cout << "Time = " << all_time.count() << endl;
	
    image.saveJPEG("raytracing.jpg");

    return 0;
}