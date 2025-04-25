#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <set>
#include <algorithm>
#include <random>
#include <iterator>
#include <numeric>
#include <chrono>
#include <iomanip> // Include header for std::setprecision and std::fixed
#include "libxl.h"

using namespace libxl;

// Define a struct for the result that includes an integer and a double, allowing multiple results to be returned (minimum fault injection rounds and average nibble faults for recovering 64 S-box values in a single experiment)
struct Result {
    int returnFaultRound;
    double returnFaultNibble;
};

// Example of the Ascon S-box (SYCON variant)
int Ascon[32] = { 8,19,30,7,6,25,16,13,22,15,3,24,17,12,4,27,11,0,29,20,1,14,23,26,28,21,9,2,31,18,10,5 };

int Sbox[64] = { 0 };     // Store the correct input values for Ascon's finalization S-box in the last round
int f_Sbox[64] = { 0 };   // Store the incorrect input values for Ascon's finalization S-box after injecting a fault (kicked off 3rd bit)
int fault[64] = { 0 };    // Store the injected nibble fault values for Ascon's finalization S-box

// Function to set the S-box with random values
void set_Sbox(int Sbox[]) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(0, 31);

    // Generate random values and mod them by 32 to populate the array
    for (int i = 0; i < 64; ++i) {
        int randomNum = distribution(gen);
        Sbox[i] = randomNum;
    }
}

// Function to set the fault array with random values
void set_fault(int F[]) {
    std::random_device rd;
    std::mt19937 gen(rd());

    // Uniform fault injection
    std::uniform_int_distribution<int> distribution(0, 31);
    for (int i = 0; i < 64; ++i) {
        int randomNum = distribution(gen);
        F[i] = randomNum;
    }

    // Uncomment for custom fault injection (probability distribution)
    /*
    std::vector<int> customDistribution = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 0: 1/6
        1, 2, 4, 8, 16, 1, 2, 4, 8, 16,   // 1/6 * 1/5
        3, 5, 6, 9, 10, 12, 17, 18, 20, 24,   // 1/6 * 1/10
        7, 11, 13, 14, 19, 21, 22, 25, 26, 28, // 1/6 * 1/10
        15, 23, 27, 29, 30, 15, 23, 27, 29, 30, // 1/6 * 1/5
        31, 31, 31, 31, 31, 31, 31, 31, 31, 31  // 1/6
    };

    std::uniform_int_distribution<int> distribution(0, customDistribution.size() - 1);
    for (int i = 0; i < 64; ++i) {
        int randomIndex = distribution(gen);
        F[i] = customDistribution[randomIndex];
    }
    */
}

// Function to calculate the intersection of two sets
std::vector<int> calculateIntersection(const std::vector<int>& set1, const std::vector<int>& set2) {
    std::vector<int> intersection;

    std::set_intersection(
        set1.begin(), set1.end(),
        set2.begin(), set2.end(),
        std::back_inserter(intersection)
    );

    return intersection;
}

// Function to perform the Ascon fault injection trial
Result Ascon_trial(Sheet* sheet, int Num) {
    int out2;
    int out3;

    // Initialize arrays for differential calculations
    std::vector<std::vector<std::vector<int>>> differ_LSB_2(32, std::vector<std::vector<int>>(4));
    std::vector<std::vector<std::vector<int>>> differ_LSB_3(32, std::vector<std::vector<int>>(4));

    // Solve the differential equation
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int in = 0; in < 32; in++) {
                out2 = Ascon[in] ^ Ascon[i ^ in];
                if (j == out2) {
                    differ_LSB_2[i][j % 4].push_back(in);
                }
                out3 = Ascon[in] ^ Ascon[i & in];
                if (j == out3) {
                    differ_LSB_3[i][j % 4].push_back(in);
                }
            }
            std::sort(differ_LSB_2[i][j % 4].begin(), differ_LSB_2[i][j % 4].end());
            std::sort(differ_LSB_3[i][j % 4].begin(), differ_LSB_3[i][j % 4].end());
        }
    }

    // Set the S-box
    set_Sbox(Sbox);

    std::wstring S;
    for (int i = 0; i < 64; ++i) {
        S += std::to_wstring(Sbox[i]) + L",";
    }
    sheet->writeStr(Num, 1, S.c_str());

    const int COUNT = 100;  // Number of fault injections
    int count;
    int temp = 0;

    // Arrays to store the intersection results
    std::vector<std::vector<std::vector<int>>> Intersection(COUNT, std::vector<std::vector<int>>(64));
    std::vector<std::vector<int>> Intersec(64);

    std::wstring Count_xor;
    std::wstring Count_and;
    std::wstring Count_all;

    // Arrays to track the fault injection counts
    int Countxor[64] = { 0 };
    int Countand[64] = { 0 };

    for (count = 0; count < COUNT; count++) {
        set_fault(fault);

        std::wstring f;
        for (int i = 0; i < 64; ++i) {
            f += std::to_wstring(fault[i]) + L",";
        }
        sheet->writeStr(Num, 5 + count * 3, f.c_str());

        // Inject faults into the S-box
        for (int i = 0; i < 64; ++i) {
            if ((Intersec[i].size() == 2) && (count > 1)) {
                f_Sbox[i] = Sbox[i] & fault[i];
            }
            else {
                f_Sbox[i] = Sbox[i] ^ fault[i];
            }
        }

        // Print the S-box output difference
        std::wstring dif;
        for (int i = 0; i < 64; ++i) {
            dif += std::to_wstring(Ascon[Sbox[i]] ^ Ascon[f_Sbox[i]]) + L",";
        }
        sheet->writeStr(Num, 6 + count * 3, dif.c_str());

        // Fill the intersection arrays
        for (int i = 0; i < 64; ++i) {
            if ((Intersec[i].size() == 2) && (count > 1)) {
                Intersection[count][i] = differ_LSB_3[fault[i]][(Ascon[Sbox[i]] ^ Ascon[f_Sbox[i]]) % 4];
            }
            else {
                Intersection[count][i] = differ_LSB_2[fault[i]][(Ascon[Sbox[i]] ^ Ascon[f_Sbox[i]]) % 4];
            }
        }

        // Initialize the intersection array on the first fault injection
        std::wstring Sb;
        if (count == 0) {
            for (int i = 0; i < 64; ++i) {
                Intersec[i] = Intersection[0][i];
                Sb += L"{";
                for (int j = 0; j < Intersec[i].size(); ++j) {
                    Sb += std::to_wstring(Intersec[i][j]) + L" ";
                }
                Sb += L"},";

                sheet->writeStr(Num, 7, Sb.c_str());
            }
        }

        // Perform intersection on subsequent fault injections
        std::wstring Sbb;
        if (count > 0) {
            for (int i = 0; i < 64; ++i) {
                Intersec[i] = calculateIntersection(Intersection[count][i], Intersec[i]);
                Sbb += L"{";
                for (int j = 0; j < Intersec[i].size(); ++j) {
                    Sbb += std::to_wstring(Intersec[i][j]) + L" ";
                }
                Sbb += L"},";

                sheet->writeStr(Num, 7 + count * 3, Sbb.c_str());
                temp += Intersec[i].size();

                if ((Intersec[i].size() == 2) && (Countxor[i] == 0)) {
                    Countxor[i] = count + 1;
                }
                if ((Intersec[i].size() == 1) && (Countand[i] == 0)) {
                    Countand[i] = count + 1 - Countxor[i];
                }
            }

            if (temp == 64) {
                break;
            }
            else {
                temp = 0;
            }
        }
    }

    int sum1 = 0;
    int sum2 = 0;
    int sum = 0;
    for (int i = 0; i < 64; ++i) {
        Count_xor += std::to_wstring(Countxor[i]) + L",";
        sum1 += Countxor[i];
        Count_and += std::to_wstring(Countand[i]) + L",";
        sum2 += Countand[i];
        Count_all += std::to_wstring(Countand[i] + Countxor[i]) + L",";
        sum += Countand[i] + Countxor[i];
    }
    double Average_Nibble1 = double(sum1) / 64;
    Count_xor += L"\nAverage number of faults for random-xor for 64 S-boxes: " + std::to_wstring(Average_Nibble1);
    double Average_Nibble2 = double(sum2) / 64;
    Count_and += L"\nAverage number of faults for random-and for 64 S-boxes: " + std::to_wstring(Average_Nibble2);
    double Average_all = double(sum) / 64;
    Count_all += L"\nAverage total faults for 64 S-boxes: " + std::to_wstring(Average_all);

    // Output results to Excel
    sheet->writeStr(Num, 2, Count_all.c_str());
    sheet->writeStr(Num, 3, Count_xor.c_str());
    sheet->writeStr(Num, 4, Count_and.c_str());

    Result result;
    result.returnFaultRound = count + 1;
    result.returnFaultNibble = Average_all;

    return result;
}

int main() {
    // Start the timer
    auto start = std::chrono::high_resolution_clock::now();

    const int trial_Num = 100;  // Number of trials in each group of experiments
    int Count[trial_Num] = { 0 };
    double Countnibble[trial_Num] = { 0 };
    int temp = 0;
    double temp1 = 0;

    // Create an Excel document object
    libxl::Book* book = xlCreateBook();
    book->setKey(L"libxl", L"windows-28232b0208c4ee0369ba6e68abv6v5i3");
    if (book) {
        libxl::Sheet* sheet = book->addSheet(L"Sheet1");

        // Set the header for the table
        sheet->writeStr(0, 1, L"5-bit S-box values");
        sheet->writeStr(0, 2, L"Total fault injection rounds to recover each S-box input value");
        sheet->writeStr(0, 3, L"Total random-xor fault injections for each S-box input value");
        sheet->writeStr(0, 4, L"Total random-and fault injections for each S-box input value");

        for (int i = 0; i < 100; ++i) {
            std::wstring i_str = std::to_wstring(i + 1);

            std::wstring output_str_1 = L"Experiment " + i_str + L" 5-bit fault values";
            std::wstring output_str_2 = L"Experiment " + i_str + L" S-box output differences";
            std::wstring output_str_3 = L"Experiment " + i_str + L" S-box possible input value sets";

            sheet->writeStr(0, 5 + 3 * i, output_str_1.c_str());
            sheet->writeStr(0, 6 + 3 * i, output_str_2.c_str());
            sheet->writeStr(0, 7 + 3 * i, output_str_3.c_str());
        }

        for (int i = 0; i < trial_Num; ++i) {
            std::wstring i_str = std::to_wstring(i + 1);
            std::wstring output_str = L"Experiment #" + i_str + L":";
            sheet->writeStr(i + 1, 0, output_str.c_str());

            Result result = Ascon_trial(sheet, i + 1);
            Count[i] = result.returnFaultRound;
            Countnibble[i] = result.returnFaultNibble;
            temp += Count[i];
            temp1 += Countnibble[i];
        }

        // End the timer
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double Average = double(temp) / double(trial_Num);
        double Averagenibble = double(temp1) / double(trial_Num);
        std::wstring newStr = L"Average fault injection rounds: " + std::to_wstring(Average) + L" Average fault nibble: " + std::to_wstring(Averagenibble) + L" Total time: " + std::to_wstring(duration.count() / 1'000'000.0) + L"s.";
        std::cout << Average << std::endl;
        std::cout << Averagenibble << std::endl;
        sheet->writeStr(0, 0, newStr.c_str());

        // Save the Excel file
        book->save(L"Ascon_random-mix_trials.xlsx");

        // Release resources
        book->release();
        std::cout << "Excel file generated successfully!" << std::endl;
    }
    else {
        std::cerr << "Unable to create Excel document object" << std::endl;
    }

    system("pause");
    return 0;
}
