#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>   // rand
#include <math.h>    // sqrt, pow, constant e
#include <limits>    // numeric_limits<float>::max()
#include <ctime>
#include <algorithm>
#include <vector>
#include <utility>
using std::string; using std::cin;
using std::cout; using std::endl;
using std::ifstream; using std::istream;
using std::clog; using std::swap;
using std::numeric_limits; using std::next_permutation;
using std::vector; using std::pair;

clock_t startTime;
clock_t actualTime;

class City
{
private:
    friend class SimulatedAnnealing;
    int cityNum;
    double x;
    double y;
public:
    City(int cityNum, double x, double y) : cityNum(cityNum), x(x), y(y) { }
    City() { }
};

class SimulatedAnnealing
{
private:
    int simListLength;
    int numCities;
    int numIterations;
    int numMaxTries;
    City **globalSolution;
    City **cities;
    int **tabuList;
    bool initialized;
    int numTriesInIteration;
    int phaseOneIters;
    int mediumSearchBound;
    int fullSearchBound;

    double endTemperatureBound;
    double temperature;
    double annealingFactorLongSearch;
    double annealingFactorRandSearch;
    int longSearchBound;
    int maxExecutionTime;
public:
    SimulatedAnnealing(int numIterations, int numMaxTries); // initialization
    ~SimulatedAnnealing();
    void furtherInit(istream &in);
    void init(string &file);
    City** genGreedySolution(City **solution, bool initialize); // generates greedy entry solution
    City** genRandomSolution(City **solution, bool initialize);
    double calcAllDist(City **cityLabel) const; // returns distance of the whole route
    double calcEdgeDist(City *a, City *b) const; // returns distance between two cities
    float calcNewDist( City **solution, int firstPair, int secondPair, double oldDist) const;
    void calcTabuIters(City **localSolution, int i, int &tabuIFirst, int &tabuISec);
    void calcTabuIters2(City **localSolution, int i, int j, int &tabuIFirst, int &tabuISec);
    float checkAllPermutations();
    void printBestSolution() const;
    void simmulatedAnnealing();
    void endComputations(City **solution);
};

inline void SimulatedAnnealing::endComputations(City **solution)
{
    cout << calcAllDist(solution) << endl;
    cout << "END COMPUTATIONS" << endl;
   // for(int i=0; i<numCities+1; ++i)
   //     clog << solution[i]->cityNum << " ";
    exit(0);
}

inline void SimulatedAnnealing::printBestSolution() const
{
    for(int i=0; i<numCities+1; ++i)
        clog << globalSolution[i]->cityNum << " ";
}

inline SimulatedAnnealing::SimulatedAnnealing(int numIterations, int numMaxTries)
{
    this->numIterations = numIterations;
    this->numMaxTries = numMaxTries;
    initialized = false;
}

SimulatedAnnealing::~SimulatedAnnealing()
{
    if(initialized)
    {
        for(int i=0; i<numCities-1; ++i)
        {
            delete cities[i];
            delete tabuList[i];
        }
        delete cities[numCities-1]; delete cities[numCities];
        delete []globalSolution;
        delete []tabuList;
        delete []cities;
    }

}

void SimulatedAnnealing::init(string &fileName)
{
    if(!fileName.empty())
    {
        ifstream file(fileName.c_str());
        if(file.good())
        {
            furtherInit(file);
            file.close();
        }
        else
            clog << "error opening file" << endl;
    }
    else
       furtherInit(cin);
}

void SimulatedAnnealing::furtherInit(istream &file)
{
        file >> numCities;
        simListLength = 2*numCities;
        if(simListLength <= 7) simListLength = 3;
        if(numCities > 20)
            mediumSearchBound = numCities/20;
        else
            mediumSearchBound = 5;
        fullSearchBound = numCities;
       // phasteThreeIters = 2*numCities;
        if(numCities >= 6000)
        {
            temperature = 101;
            annealingFactorLongSearch = 5;
            annealingFactorRandSearch = 0.2;
            longSearchBound = 60;
            phaseOneIters = 1*numCities;
            fullSearchBound = 2*numCities; // so it never does it
        }
        else
        {
            temperature = 400;
            longSearchBound = 200;
            annealingFactorLongSearch = 1;
            annealingFactorRandSearch = 1;
            phaseOneIters = 6*numCities;
        }
        endTemperatureBound = 0;

        globalSolution = new City*[numCities+1];
        cities = new City*[numCities+1];
        tabuList = new int *[numCities-1];
        for(int i=0; i<numCities-1; ++i)
        {
            tabuList[i] = new int[numCities-1-i];
            for(int j=0; j<numCities-1-i; ++j)
                tabuList[i][j] = 0;
        }

        int cityNum;
        double x, y;
        for(int i=0; i<numCities; ++i)
        {
           file >> cityNum >> x >> y;
           cities[i] = new City(cityNum, x, y);
        }
        cities[numCities] = cities[0];
        file >> maxExecutionTime;
        maxExecutionTime = float(maxExecutionTime) * 0.9;
        initialized = true;
}

inline double SimulatedAnnealing::calcEdgeDist(City *a, City *b) const
{
    return sqrt(pow((a->x - b->x), 2) + pow((a->y - b->y), 2));
}

inline double SimulatedAnnealing::calcAllDist(City **cityLabel) const
{
    double dist = 0.0;
    for(int i=0; i<numCities; ++i) // go through all edges, city's length is numCities+1
    {
        dist += calcEdgeDist(cityLabel[i], cityLabel[i+1]);
    }
    return dist;
}

// generate greedy solution based on Cities **cities
City** SimulatedAnnealing::genGreedySolution(City **solution, bool initialize)
{
    if(initialize)
    {
        for(int i=0; i<numCities+1; ++i)
            solution[i] = cities[i];
    }

    float minDist, tempDist;
    int cityNum;
    for(int i=0; i<numCities-1; ++i)
    {
        minDist = calcEdgeDist(solution[i], solution[i+1]);
        cityNum = i+1;
        for(int j=i+2; j<numCities; ++j)
        {
            tempDist = calcEdgeDist(solution[i], solution[j]);
            if(tempDist < minDist)
            {
                minDist = tempDist;
                cityNum = j;
            }
        }
        swap(solution[i+1], solution[cityNum]);
    }
    solution[0] = solution[numCities] = cities[0]; // starting and ending point is always labbel 1
    return solution;
}

City **SimulatedAnnealing::genRandomSolution(City **solution, bool initialize)
{
    // we can shuffle values from previous iterations
    if(initialize)
    {
        for(int i=0; i<numCities+1; ++i)
            solution[i] = cities[i];
    }
    // shuffle elements in array, without first and last element
    int r;
    for(int i=1; i<numCities-1; ++i)
    {
        r = rand()%(numCities - i - 1) + i+1; // rand in range [i+1, numCities)
        swap(solution[i], solution[r]);
    }

//    for(int i=0; i<numCities+1; ++i)
//        cout << solution[i] << " ";
//    cout << endl;
    return solution;
}

// calculates distance after swaping two edge pairs
inline float SimulatedAnnealing::calcNewDist(City **solution, int firstPair, int secondPair, double oldDist) const
{
    --firstPair; // changed from previous solution when I passed [i, i+1] and firstPair was equal i, now I pass i+1, kek, i dont want to change all other stuff for now
    if(firstPair+1 == secondPair-1) // for example: 1->2->3->4 changes to 1->3->2->4
        return (oldDist - calcEdgeDist(solution[firstPair], solution[firstPair+1]) -\
                calcEdgeDist(solution[secondPair], solution[secondPair+1]) +\
                calcEdgeDist(solution[firstPair], solution[secondPair]) +\
                calcEdgeDist(solution[firstPair+1], solution[secondPair+1]));
    else // 1->2->3->4->5 changes to 1->4->3->2->5
        return(oldDist - calcEdgeDist(solution[firstPair], solution[firstPair+1]) -\
                calcEdgeDist(solution[firstPair+1], solution[firstPair+2]) -\
                calcEdgeDist(solution[secondPair-1], solution[secondPair]) -\
                calcEdgeDist(solution[secondPair], solution[secondPair+1]) +\
                calcEdgeDist(solution[firstPair], solution[secondPair]) +\
                calcEdgeDist(solution[secondPair], solution[firstPair+2]) +\
                calcEdgeDist(solution[secondPair-1], solution[firstPair+1]) +\
                calcEdgeDist(solution[firstPair+1], solution[secondPair+1]));
}

inline void SimulatedAnnealing::calcTabuIters(City **localSolution, int i, int &tabuIFirst, int &tabuISec)
{
    if( localSolution[i]->cityNum < localSolution[i+1]->cityNum )
    {
        tabuIFirst = localSolution[i]->cityNum - 1;
        tabuISec = numCities - localSolution[i+1]->cityNum;
    }
    else
    {
        tabuIFirst = localSolution[i+1]->cityNum - 1;
        tabuISec = numCities - localSolution[i]->cityNum;
    }
}

inline void SimulatedAnnealing::calcTabuIters2(City **localSolution, int i, int j, int &tabuIFirst, int &tabuISec)
{
    if( localSolution[i]->cityNum < localSolution[j]->cityNum )
    {
        tabuIFirst = localSolution[i]->cityNum - 1;
        tabuISec = numCities - localSolution[j]->cityNum;
    }
    else
    {
        tabuIFirst = localSolution[j]->cityNum - 1;
        tabuISec = numCities - localSolution[i]->cityNum;
    }
}


void SimulatedAnnealing::simmulatedAnnealing()
{
    City **localSolution = new City*[numCities+1];
    double globalSolutionDist, localSolutionDist;

    double tempDist;
    double currentTemperature;


    int whichTry;
    int firstCity;
    int secondCity;
    double bestLocalSolutionDist;

    // dodanie wszystkich indeksow mozliwych zamian krawedziowych
    vector< pair<short int,short int> > allEdges;

    allEdges.reserve((numCities-1)*(numCities-2)/2);
    for(int i=1; i<numCities-1; ++i)
        for(int j=i+1; j<numCities; ++j)
            allEdges.push_back(std::make_pair(i,j));

//    localSolution = genGreedySolution(localSolution, true);
//    for(int i=0; i<numCities+1; ++i)
//        globalSolution[i] = bestLocalSolution[i] = localSolution[i];
//   // cout << "GREEDY: " << calcAllDist(globalSolution) << endl;
//    globalSolutionDist = bestLocalSolutionDist = calcAllDist(localSolution);
//     whichTry = 0;

    for(int tries=0; tries<numMaxTries; ++tries)
    {
        //++tries;
       // cout << "tries: " << tries << endl;
        // initialize best global solution, and local solutions
        currentTemperature = temperature;
        if(tries == 0)
        {
            localSolution = genGreedySolution(localSolution, true);
            for(int i=0; i<numCities+1; ++i)
                globalSolution[i] = localSolution[i];
            cout << "GREEDY: " << calcAllDist(globalSolution) << endl;
            localSolutionDist = globalSolutionDist = calcAllDist(localSolution);
            whichTry = 0;
        }
        else
        {
            localSolution = genRandomSolution(localSolution, true);
            localSolutionDist = calcAllDist(localSolution);
            cout << "STARTING SOLUTION: " << calcAllDist(localSolution) << endl;
        }
        // calculate overall distance in newly created solution

        //cout << "current dist: " << currentDist << endl;


        numTriesInIteration = (3/2) * numCities; // starting number of tries in iteration
        int maxTriesInIteration = 4*numCities;


        /* PHASE ONE */

        while(currentTemperature > endTemperatureBound) //TODO zmienic warunek zakonczenia
        {
            int k =0;
            int h=0;
            // just a random initialization in case no solution would be found

            if(currentTemperature < longSearchBound) //TODO zmienic warunek
            {   //cout << "TEMP1" << endl;
                for(int i=1; i<numCities-1; ++i) // go through all possible two edge swaps, last pair is [numCities-3, numCities-2] and [numCities-1,numCities]
                {
                    bestLocalSolutionDist = numeric_limits<double>::max();
                    for(int j= i+1; j<numCities; ++j) // go through all ending pairs, last one is [numCities-1, numCities]
                    {
                        // find new solution
                        tempDist = calcNewDist(localSolution, i, j, localSolutionDist);
                        //cout << "tempDist: " << tempDist << endl;
                        // if new solution is better then take it
                        //cout << "DIFF: " << tempDist - localSolutionDist << endl;
                        if(tempDist < bestLocalSolutionDist)
                        {
                            bestLocalSolutionDist = tempDist;
                            firstCity = i;
                            secondCity = j;
//                            if(tempDist < globalSolutionDist)
//                            {
//                                globalSolutionDist = tempDist; // remember it as best one
//                                //swap(globalSolution[i], globalSolution[j]);
//                            }
                        }
//                        else
//                        {
//                            double a = (double(rand()) /double(RAND_MAX));
//                            //double b = 1.0 / (1.0 + exp(float(tempDist - localSolutionDist)/temperature));
//                            double b = exp(float(-tempDist + localSolutionDist)/temperature);
//                            if(a < b)
//                            {
//                                if(++k < 50)
//                                cout << a << " :lol1: " << b << " ,DELTA: " << -tempDist + localSolutionDist << endl;
//                                //cout << 1.0 / (1.0 + exp(float(tempDist - localSolutionDist)/temperature)) << endl;
//                                localSolutionDist = tempDist;
//                                swap(localSolution[i], localSolution[j]);
//                            }
//                        }

                     }
                    if(bestLocalSolutionDist < localSolutionDist)
                    {
                        swap(localSolution[firstCity], localSolution[secondCity]);
                        localSolutionDist = bestLocalSolutionDist;
                    }
                    else
                    {
                        double a = (double(rand()) /double(RAND_MAX));
                        double b = exp(double(-bestLocalSolutionDist + localSolutionDist)/temperature);
                        //cout << bestLocalSolutionDist << " wow " << localSolutionDist << endl;
                        if( (a < b) && (bestLocalSolutionDist != localSolutionDist) )
                        {
                            swap(localSolution[firstCity], localSolution[secondCity]);
                            //cout << a << " :lol1: " << b << " ,DELTA: " << -bestLocalSolutionDist + localSolutionDist << endl;
                            localSolutionDist = bestLocalSolutionDist;
                        }
                    }
                    actualTime = clock();
                    if( (actualTime - startTime)/CLOCKS_PER_SEC >= maxExecutionTime) endComputations(localSolution);

                }
            }
            else
            {
                int maxRandSize = allEdges.size();
                int r = rand()%(maxRandSize);

                // shufflowanie numTriesInIteration wynikow i wybranie ich jako losowe
                for(int i=0; i<numTriesInIteration; ++i)
                {
                    r = rand()%(maxRandSize - i) + i; // rand in range [i, maxRandSize)
                    swap(allEdges[i], allEdges[r]);
                }
                for(int i=0; i<numTriesInIteration; ++i)
                {
                    r = rand()%(maxRandSize - i) + i; // rand in range [i, maxRandSize)
                    swap(allEdges[i], allEdges[r]);
                }
                bestLocalSolutionDist = numeric_limits<double>::max();
                for(int i=0; i<numTriesInIteration; ++i)
                {
                    //find best possible solution in current iteration, it can be worse than our best solution from previous iteration,
                    // but global variable will remember best one so far in a try
                    tempDist = calcNewDist(localSolution, allEdges[i].first, allEdges[i].second, localSolutionDist);

                    //cout << "tempDist: " << tempDist << endl;
                    if(tempDist < bestLocalSolutionDist)
                    {
                        bestLocalSolutionDist = tempDist;
                        firstCity = allEdges[i].first;
                        secondCity = allEdges[i].second;
                    }

                }

                if(bestLocalSolutionDist < localSolutionDist)
                {
                    swap(localSolution[firstCity], localSolution[secondCity]);
                    localSolutionDist = bestLocalSolutionDist;
                }
                else
                {
                    double a = (double(rand()) /double(RAND_MAX));
                    double b = exp(double(-bestLocalSolutionDist + localSolutionDist)/temperature);
                    if( (a < b) && (localSolutionDist != bestLocalSolutionDist) )
                    {
                        swap(localSolution[firstCity], localSolution[secondCity]);
                        //cout << a << " :lol2: " << b << " ,DELTA: " << -bestLocalSolutionDist + localSolutionDist << endl;
                        localSolutionDist = bestLocalSolutionDist;
                    }
                }

                actualTime = clock();
                if( (actualTime - startTime)/CLOCKS_PER_SEC >= maxExecutionTime) endComputations(localSolution);
                if(numTriesInIteration < maxTriesInIteration) numTriesInIteration += 0.1 * numCities;
                //cout << "TEMP" << endl;
             }

            // update best local solution if found one
//            if(localSolutionDist < bestLocalSolutionDist)
//            {
//             //   cout << "best local: " << localSolutionDist << endl;
//                bestLocalSolutionDist = localSolutionDist;
//                for(int i=0; i<numCities+1; ++i)
//                    bestLocalSolution[i] = localSolution[i];
//            }
            if(currentTemperature < longSearchBound)
                currentTemperature = currentTemperature - annealingFactorLongSearch; // TODO update temperature in each iteration
            else
                currentTemperature = currentTemperature - annealingFactorRandSearch;
        }

//        if(bestLocalSolutionDist < globalSolutionDist)
//        {
//           // cout << "BEST GLOBAL: " << bestLocalSolutionDist << endl;
//            globalSolutionDist = bestLocalSolutionDist;
//            for(int i=0; i<numCities+1; ++i)
//                globalSolution[i] = bestLocalSolution[i];
//            whichTry = tries;
//        }

        cout << "LOCAL DIST: " << calcAllDist(localSolution) << endl;
        cout << "GLOBAL DIST: " << globalSolutionDist << endl;
        cout << "\n\n";
    }
//    cout << "global solution: " << globalSolutionDist << endl;
//    cout << "assert: " << calcAllDist(globalSolution) << endl;
//    cout << "whichTry: " << whichTry << endl;

//    for(int i=0; i<numCities-1; ++i)
//    {
//        for(int j=0; j<numCities-1-i; ++j)
//            cout << tabuList[i][j] << " ";
//        cout << endl;
//    }

    delete []localSolution;
}

float SimulatedAnnealing::checkAllPermutations()
{
    float minDist = numeric_limits<float>::max();
    float tmpDist;
    int count=0;
    std::sort(globalSolution+1, globalSolution + numCities);

    while(next_permutation(globalSolution+1, globalSolution + numCities))
    {
        tmpDist = calcAllDist(globalSolution);
        if(tmpDist < minDist)
            minDist = tmpDist;

        if(count==20000000) {count=0; cout << "min so far: " << minDist << endl;}
        ++count;
    }

    return minDist;
}

/*
 * f(x_nowe) > f(x_najlepsze_rozw) - Epsilon * H(x) ( H(x) = kara, wieksza tym bardziej im wczesniej byla zamiana tabu)
 * vec np. (1,2) (1,3) (1,4) (1,5) (2,3) (2,4) (2,5) ... (4,5)
 * vec_copy = vec
 * losujemy x_nowe, vec_copy -= x_nowe, wiec losowanie nastepnego nie bedzie trafialo na te same mozliwosci
 * agresywne zawezanie przez usuwanie np. wybierzemy (2,3) -> usuwanie wszystkich swapow z 2, no albo tylko te (2,3)
 * przechowywanie tabu, kiedy ostatnio zmienilismy swapa konkretnego i porownywanie w biegu
 *
 * przy symulowanym wyrzazaniu zmiana temperatury co iteracji o mala wartosc a co np 15 iteracji o wieksza wartosc
 */



int main()
{
//       int temp2 = 140;
//       while(temp2 > 1)
//       {
//           cout << temp2 << endl;
//           temp2 = temp2*0.95;
//       }

//    int delEnd = 0;
//    int temp = 1;
//    while(temp < 70)
//    {
//        int delPocz = -200;
//        while(delPocz < delEnd)
//        {
//            cout << "TEMP: " << temp << ", DELTA: " << delPocz << ", PROC: " << exp(float(delPocz)/temp) << endl;
//            delPocz += 10;
//        }
//        temp += 5;
//    }


    startTime = clock();
    srand(time(NULL));

    string fileName = "dane.txt"; //TODO WYWALIC STRINGA NA NULLA XD
    SimulatedAnnealing sim(123,5);
    sim.init(fileName);
    //cout << "wczytalem" << endl;
    sim.simmulatedAnnealing();
    //sim.printBestSolution();
    cout << "\n\n\n\n\n";

  //  int temp = 1;




//    int temp = 8;
//    while(temp > 1)
//    {
//        cout << temp << endl;
//        temp = temp*0.95;
//    }


   // float wynik = tabu.checkAllPermutations();
    //cout << wynik << endl;
    return 0;
}
