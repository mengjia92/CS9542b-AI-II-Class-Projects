// This program computes the log probability of an input file using the Add-Delta model (n-gram mode) built from the training file
// Example command line: P4 textModel.txt sentences.txt 5 0.1

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>
#include <cmath>
#include <limits>

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


int getCount(ngramMap& data, vector<T> w)
{
	double C = 0;

	if (data.count(w) > 0) {
		C = data.at(w);
	}
	else {
		C = double(0);
	}

	return double(C);
}


double addDelta(double C, double delta, double n, double B)
{
	double p = 0;

	p = (C + delta) / (n + delta*B);

	return p;
}


void main(int argc, char **argv) {
	int N;
	double delta;
	string inputFile1, inputFile2;

	cout << "P4 ";
	cin >> inputFile1 >> inputFile2 >> N >> delta;

	vector<T> sentence, w_top, w_bottom;
	double c_top = 0, c_bottom = 0, p_top = 0, p_bottom = 0, probs = 0;


	read_tokens(inputFile2, sentence, false); // reads words from file without EOS 

	vector<unordered_map<vector<T>, int>> database;
	for (unsigned int i = 0; i < N; i++)
	{
		unordered_map<vector<T>, int> nGram;
		makeNgram(nGram, inputFile1, i + 1);

		database.push_back(nGram);
	}

	double fileSize = accumulate(begin(database[0]), end(database[0]), 0
		, [](int value, const unordered_map<vector<T>, int>::value_type& p) { return value + p.second; }
	);

	double V = database[0].size() + 1;
	

	if (N == 1)
	{
		for (unsigned int j = 0; j < sentence.size(); j++) {
			w_top.push_back(sentence[j]);

			c_top = getCount(database[w_top.size() - 1], w_top);
			p_top = addDelta(c_top, delta, fileSize, pow(V, w_top.size()));

			probs += log(p_top);

			w_top.pop_back();
		}
	}
	else {
		if (sentence.size() < N) {
			cout << "\nInput file is too small to create any nGrams of size " << N;
		}
		else {
			for (unsigned int m = 0; m < sentence.size(); m++)
			{
				if (m == 0) {
					w_top.push_back(sentence[m]);
					
					c_top = getCount(database[w_top.size() - 1], w_top);
					p_top = addDelta(c_top, delta, fileSize, pow(V, w_top.size()));

					probs += log(p_top);
				}
				else {
					w_top.push_back(sentence[m]);
					w_bottom.push_back(sentence[m - 1]);

					if (w_top.size() > N) // check the length of ngram
					{
						w_top.erase(w_top.begin());
						w_bottom.erase(w_bottom.begin());
					}

					c_top = getCount(database[w_top.size() - 1], w_top);
					p_top = addDelta(c_top, delta, fileSize, pow(V, w_top.size()));

					c_bottom = getCount(database[w_bottom.size() - 1], w_bottom);
					p_bottom = addDelta(c_bottom, delta, fileSize, pow(V, w_bottom.size()));
					
					probs += log(p_top) - log(p_bottom);
				}	
			}
		}
	}
	
	if (probs == -INFINITY) {
		cout << "\n-DBL_MAX: " << -DBL_MAX << "\n";
	}
	else {
		cout << "\n" << probs << "\n";
	}
}
