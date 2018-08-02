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

typedef char T; // for char language model

using ngramMap = unordered_map<vector<T>, int>;

void makeNgram(ngramMap& data, string inputFile, int n) {
	try {
		vector<T> tokens;

		read_tokens(inputFile, tokens, false); // reads tokens from file without EOS 
		//read_tokens(inputFile, tokens, true); 

		if (tokens.size() < n)
			cout << "\nInput file is too small to create any nGrams of size " << n;
		else
		{
			for (int i = 0; i <= tokens.size() - n; i++)
			{
				vector<T> ngram(n);

				for (unsigned int j = 0; j < n; j++) // combine words to ngram
					ngram[j] = tokens[i + j];

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


double getCount(ngramMap& data, vector<T> w)
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


double calcu_prob(vector<T> sen, unordered_map<int, unordered_map<vector<T>, int>> database
	, int n, double delta, double num_tokens, double V)
{	
	vector<T> w_top, w_bottom;
	double c_top = 0, c_bottom = 0, p_top = 0, p_bottom = 0, probs = 0;

	if (n == 1)
	{
		for (unsigned int j = 0; j < sen.size(); j++) {
			w_top.push_back(sen[j]);

			c_top = getCount(database.at(w_top.size()), w_top);
			p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));

			probs += log(p_top);

			w_top.pop_back();
		}
	}
	else {
		if (sen.size() < n) {
			cout << "\nThe sentence is too small to create any nGrams of size " << n;
		}
		else {
			for (unsigned int m = 0; m < sen.size(); m++)
			{
				if (m == 0) {
					w_top.push_back(sen[m]);

					c_top = getCount(database.at(w_top.size()), w_top);
					p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));

					probs += log(p_top);
				}
				else {
					w_top.push_back(sen[m]);
					w_bottom.push_back(sen[m - 1]);

					if (w_top.size() > n) // check the length of ngram
					{
						w_top.erase(w_top.begin());
						w_bottom.erase(w_bottom.begin());
					}

					c_top = getCount(database.at(w_top.size()), w_top);
					p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));

					c_bottom = getCount(database.at(w_bottom.size()), w_bottom);
					p_bottom = addDelta(c_bottom, delta, num_tokens, pow(V, w_bottom.size()));

					probs += log(p_top) - log(p_bottom);
				}
			}
		}
	}

	if (probs == -INFINITY) {
		return -DBL_MAX;
	}
	else {
		return probs;
	}
}


void main(int argc, char **argv) {
	int n;
	double delta, senLen;

	vector<string> train = { "danish1.txt", "english1.txt", "french1.txt", "italian1.txt", "latin1.txt", "sweedish1.txt"};
	vector<string> test = { "danish2.txt", "english2.txt", "french2.txt", "italian2.txt", "latin2.txt", "sweedish2.txt"};

	cout << "P6 ";
	cin >> n >> delta >> senLen;

	if ((n == 0) || (senLen == 0)) {
		cout << "\nInvalid parameter values\n";
	}
	else {
		unordered_map<int, unordered_map<int, unordered_map<vector<T>, int>>> allLang;

		for (unsigned int i = 0; i < train.size(); i++) // build maps on training files
		{
			unordered_map<int, unordered_map<vector<T>, int>> database; // get a ngram collection

			for (unsigned int j = 0; j < n; j++)
			{
				unordered_map<vector<T>, int> nGram;
				makeNgram(nGram, train[i], j + 1);

				database[j + 1] = nGram;
			}
			
			allLang[i] = database;
		}


		////////// test models on test files //////////
		vector<int> true_label, predicted;

		double vocab = 256; // vocabulary size
		//double vocab = 26;

		for (unsigned int k = 0; k < test.size(); k++) 
		{
			vector<T> chars;

			read_tokens(test[k], chars, false);
			//read_tokens(test[k], chars, true);

			int num_of_chunks = floor(chars.size() / senLen);

			double fileSize = accumulate(begin(allLang.at(k).at(1)), end(allLang.at(k).at(1)), double(0)
				, [](int value, const unordered_map<vector<T>, int>::value_type& p) { return value + p.second; }
			);

		
			for (unsigned int m = 0; m < num_of_chunks; m++) 
			{
				true_label.push_back(k);

				vector<T> sentence(chars.begin() + m*senLen, chars.begin() + (m+1)*senLen);

				double max_prob = -INFINITY;
				int max_idx = 0;

				for (unsigned int x = 0; x < test.size(); x++) // compute the sentence probability under each language
				{
					double lang_prob = calcu_prob(sentence, allLang.at(x), n, delta, fileSize, vocab);

					if (lang_prob > max_prob) {
						max_prob = lang_prob;
						max_idx = x;
					}
				}
				predicted.push_back(max_idx);
			}
		}
		
		int CONF[6][6] = { 0 };
		double accu = 0;

		for (unsigned int s = 0; s < true_label.size(); s++)
		{
			CONF[true_label[s]][predicted[s]] += 1;

			if (true_label[s] == predicted[s]) {
				accu += 1;
			}
		}

		cout << "\n" << accu / true_label.size() * 100 << "%" << "\n"; // print out the results

		for (unsigned int y = 0; y < test.size(); y++) {
			cout << "\n";
			for (unsigned int z = 0; z < test.size(); z++) {
				cout << CONF[y][z] << " ";
			}
		}
	}
}