// N-gram random sentence generation using maximum likelihood algorithm
// Example command line: P3 text.txt 3

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>

using namespace std;

#include "fileRead.h"
#include "VectorHash.h"
#include "utilsToStudents.h"

typedef string T;
using ngramMap = unordered_map<vector<T>, int>;

void makeNgram(ngramMap& data, string inputFile, int n) {
	try {
		vector<T> words;

		read_tokens(inputFile, words, true); // reads tokens from file without EOS 

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


void getSentence(vector<double> p, vector<pair<vector<T>, int>> words)
{
	vector<T> sentence;

	// get the indices of words
	int first_idx = drawIndex(p);

	vector<T> first_word = words[first_idx].first;

	if (first_word[0] == "<END>")
	{
		cout << "The first word is <END>, please restart the program" << "\n";
	}
	else {
		sentence.push_back(first_word[0]);

		while (first_word[0] != "<END>")
		{
			int idx = drawIndex(p);
			vector<T> word = words[idx].first;

			if (word[0] != "<END>")
			{
				sentence.push_back(word[0]);
			}
			else {
				sentence.push_back("<END>");
				break;
			}
		}
	}

	// pring out the whole sentence
	for (unsigned int j = 0; j < sentence.size(); j++)
		cout << sentence[j] << " ";
	cout << "\n";
}


void main(int argc, char **argv)
{
	string inputFile;
	int N;

	cout << "P3 ";
	cin >> inputFile >> N;

	srand(time(NULL));

	// make a map collection of nGram maps
	vector<unordered_map<vector<T>, int>> database;

	for (unsigned int i = 0; i < N; i++)
	{
		unordered_map<vector<T>, int> nGram;
		makeNgram(nGram, inputFile, i + 1);

		database.push_back(nGram);
	}


	double fileSize = accumulate(begin(database[0]), end(database[0]), double(0)
		, [](int value, const unordered_map<vector<T>, int>::value_type& p) { return value + p.second; }
	);

	// dump words dict to vector of pairs
	vector<pair<vector<T>, int>> Words;

	copy(database[0].begin(), database[0].end(), back_inserter(Words));


	// calculate probabilities
	vector<double> probs(database[0].size());

	if (N == 1)
	{
		for (unsigned int m = 0; m < probs.size(); m++)
		{
			probs[m] = double(Words[m].second) / fileSize;
		}

		getSentence(probs, Words);
	}

	else {
		vector<T> sentence, w_top, w_bottom;
		double c_top = 0, c_bottom = 0;

		w_bottom.push_back("<END>");
		w_top = w_bottom;

		for (unsigned int m = 0; m < probs.size(); m++)
		{
			vector<T> each_word = Words[m].first;

			w_top.push_back(each_word[0]); // the top add a new word

			c_top = getCount(database[w_top.size() - 1], w_top); // get count from related nGram map
			c_bottom = getCount(database[w_bottom.size() - 1], w_bottom);

			probs[m] = double(c_top) / double(c_bottom);

			w_top.pop_back();
		}

		// get the indices of words
		int first_idx = drawIndex(probs);

		vector<T> first_word = Words[first_idx].first;

		if (first_word[0] == "<END>")
		{
			cout << "The first word is <END>, please restart the program" << "\n";
		}
		else {
			sentence.push_back(first_word[0]);

			w_top.push_back(first_word[0]);
			w_bottom.push_back(first_word[0]);
			
			while (first_word[0] != "<END>")
			{
				for (unsigned int n = 0; n < probs.size(); n++)
				{
					vector<T> w = Words[n].first;

					w_top.push_back(w[0]); // the top add a new word

					if (w_top.size() > N) // check
					{
						w_top.erase(w_top.begin());
						w_bottom.erase(w_bottom.begin());
					}

					c_top = getCount(database[w_top.size() - 1], w_top); // get count from related nGram map
					c_bottom = getCount(database[w_bottom.size() - 1], w_bottom);

					probs[n] = double(c_top) / double(c_bottom);

					w_top.pop_back();
				}

				int idx = drawIndex(probs);
				vector<T> word = Words[idx].first;

				if (word[0] != "<END>")
				{
					sentence.push_back(word[0]);

					w_top.push_back(word[0]);
					w_bottom.push_back(word[0]);
				}
				else {
					sentence.push_back("<END>");
					break;
				}
			}
		}

		// pring out the whole sentence
		for (unsigned int j = 0; j < sentence.size(); j++)
			cout << sentence[j] << " ";
		cout << "\n";
	}
		
}
			
