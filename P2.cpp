// This program takes as the command line arguments the names of two text files, the size n for the nGram, and the last parameter that indicates whether to print out common nGrams or not. 
// It outputs the percentage of nGrams in the second text file that do not occur in the first text file. 
// Example command line: P2 text1.txt text2.txt 5 0

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include<algorithm>
#include <iterator>

using namespace std;

#include "fileRead.h"
#include "VectorHash.h"

typedef string T; // for string language model
using ngramMap = unordered_map<vector<T>, int>;

void makeNgram(ngramMap& data, string inputFile, int n) {
	try {
			vector<T> words; 

			read_tokens(inputFile, words, false); // reads tokens from file without EOS 

			if (words.size() < n)
				cout << "\nInput file is too small to create any nGrams of size " << n;
			else
			{
				for (int i = 0; i <= words.size() - n; i++)
				{
					vector<T> ngram(n);

					for (unsigned int j = 0; j < n; j++) // combine words to ngram
						ngram[j] = words[i + j];

					if (data.count(ngram) == 0)
						data[ngram] = 1;
					else
						data[ngram] = data[ngram] + 1;
				}
			}
	}

	catch (FileReadException e)
	{
		e.Report();
	}
}


void main(int argc, char **argv) {
	int N, choice;
	string inputFile1, inputFile2;
	double counter = 0;

	cout << "P2 ";
	cin >> inputFile1 >> inputFile2 >> N >> choice;

	unordered_map<vector<T>, int> nGram1;
	unordered_map<vector<T>, int> nGram2;

	makeNgram(nGram1, inputFile1, N);
	makeNgram(nGram2, inputFile2, N);

	if (nGram1.size() < nGram2.size())
	{
		for (auto it = nGram1.begin(); it != nGram1.end(); ++it)
		{
			vector<T> ngram = it->first;

			if (nGram2.count(ngram) > 0) {
				counter += 1;

				if (choice == 1) {
					cout << "\n";

					for (unsigned int j = 0; j < ngram.size(); j++) {
						cout << ngram[j] << " ";  // print out the common ngrams
					}
				}
			}
		}
	}
	else {
		for (auto it = nGram2.begin(); it != nGram2.end(); ++it)
		{
			vector<T> ngram = it->first;

			if (nGram1.count(ngram) > 0) {
				counter += 1;

				if (choice == 1) {
					cout << "\n";

					for (unsigned int j = 0; j < ngram.size(); j++) {
						cout << ngram[j] << " ";  // print out the common ngrams
					}
				}
			}
		}
	}
	
	cout << "\n" << (nGram2.size() - counter) / nGram2.size() * 100 << "%" << "\n";
}
