#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>
#include <cmath>
#include <set>

using namespace std;

#include "fileRead.h"
#include "VectorHash.h"

typedef string T; // for string language model
using ngramMap = unordered_map<vector<T>, int>;
using normalized = unordered_map<vector<T>, double>;

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


void GT(normalized& norm, vector<unordered_map<vector<T>, int>> database, double corpus, double vocab, int n, int t)
{
	int maxCount = 0;
	for (auto it = database[n-1].begin(); it != database[n-1].end(); ++it)
	{
		if (maxCount < it->second) {
			maxCount = it->second;
		}
	}

	if (t > maxCount) {
		t = maxCount;
	}

	vector<double> Nr(maxCount + 1, double(0));

	Nr[0] = pow(vocab, n) - database[n-1].size(); // unseen ngrams

	for (auto m = database[n-1].begin(); m != database[n-1].end(); ++m)
	{
		if (Nr[m->second] == 0) {
			Nr[m->second] = double(1);
		}
		else {
			Nr[m->second] += double(1);
		}
	}

	for (unsigned int a = 0; a < Nr.size(); a++) // check the threshold
	{	
		if (Nr[a] == 0)
		{
			if (a < t) {
				t = a;
				break;
			}
		}
	}


	double p_unseen = 0;

	if (t != 0) {
		p_unseen = Nr[0 + 1] / (corpus*Nr[0]); // probability of EACH unseen ngram
	}

	for (auto k = database[n-1].begin(); k != database[n-1].end(); ++k) // GT algorithm
	{
		if (k->second < t)
		{
			norm[k->first] = (double(k->second) + 1) * (Nr[double(k->second) + 1] / (corpus*Nr[double(k->second)]));
		}
		else {
			norm[k->first] = double(k->second) / corpus;
		}
	}

	double p_unseen_total = p_unseen * Nr[0];  // normalization

	double p_seen_total = accumulate(begin(norm), end(norm), double(0)
		, [](double value, const unordered_map<vector<T>, double>::value_type& p) { return value + p.second; }
	);

	double weight = (1 - p_unseen_total) / p_seen_total;

	for (auto j = norm.begin(); j != norm.end(); ++j) // apply weight to the original GT probability
	{
		j->second *= weight;
	}

	norm[{"UNKNOWN"}] = p_unseen; // add the probability of unseen ngram to map
}


void main(int argc, char **argv) {
	int n, t;
	string inputFile1, inputFile2;

	cout << "P5 ";
	cin >> inputFile1 >> inputFile2 >> n >> t;


	vector<T> sentence, w_top, w_bottom;
	double p_top = 0, p_bottom = 0, probs = 0;
	
	read_tokens(inputFile2, sentence, false); // reads words from file without EOS 

	vector<unordered_map<vector<T>, int>> database; // get a ngram collection
	for (unsigned int i = 0; i < n; i++)
	{
		unordered_map<vector<T>, int> nGram;
		makeNgram(nGram, inputFile1, i + 1);

		database.push_back(nGram);
	}

	double fileSize = accumulate(begin(database[0]), end(database[0]), double(0)
		, [](int value, const unordered_map<vector<T>, int>::value_type& p) { return value + p.second; }
	);

	double V = database[0].size() + 1;

	
	vector<unordered_map<vector<T>, double>> GTbase; // get a GT collection
	for (unsigned int j = 0; j < n; j++)
	{
		unordered_map<vector<T>, double> norm_p;
		GT(norm_p, database, fileSize, V, j+1, t);

		GTbase.push_back(norm_p);
	}

	
	if (n == 1)
	{
		for (unsigned int k = 0; k < sentence.size(); k++) {
			w_top.push_back(sentence[k]);

			if (GTbase[w_top.size() - 1].count(w_top) > 0)
			{
				p_top = GTbase[w_top.size() - 1].at(w_top);
			}
			else {
				p_top = GTbase[w_top.size() - 1].at({ "UNKNOWN" });
			}

			probs += log(p_top);

			w_top.pop_back();
		}
	}
	else {
		if (sentence.size() < n) {
			cout << "\nInput file is too small to create any nGrams of size " << n;
		}
		else {
			for (unsigned int m = 0; m < sentence.size(); m++)
			{
				if (m == 0) {
					w_top.push_back(sentence[m]);

					if (GTbase[w_top.size() - 1].count(w_top) > 0)
					{
						p_top = GTbase[w_top.size() - 1].at(w_top);
					}
					else {
						p_top = GTbase[w_top.size() - 1].at({ "UNKNOWN" });
					}

					probs += log(p_top);
				}
				else {
					w_top.push_back(sentence[m]);
					w_bottom.push_back(sentence[m - 1]);

					if (w_top.size() > n) // check the length of ngram
					{
						w_top.erase(w_top.begin());
						w_bottom.erase(w_bottom.begin());
					}

					if (GTbase[w_top.size() - 1].count(w_top) > 0) // get the probability for the top
					{
						p_top = GTbase[w_top.size() - 1].at(w_top);
					}
					else {
						p_top = GTbase[w_top.size() - 1].at({ "UNKNOWN" });
					}

					if (GTbase[w_bottom.size() - 1].count(w_bottom) > 0) // get the probability for the bottom
					{
						p_bottom = GTbase[w_bottom.size() - 1].at(w_bottom);
					}
					else {
						p_bottom = GTbase[w_bottom.size() - 1].at({ "UNKNOWN" });
					}

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
