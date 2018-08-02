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
#include "utilsToStudents.h"

typedef string T; // for string language model

using ngramMap = unordered_map<vector<T>, int>;
using normalized = unordered_map<vector<T>, double>;
using input_sen = vector<T>;


void makeNgram(ngramMap& data, string inputFile, int n) {
	try {
		vector<T> tokens;

		read_tokens(inputFile, tokens, false); // reads tokens from file without EOS 

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


void GT(normalized& norm, vector<unordered_map<vector<T>, int>> database, double corpus, double vocab, int n, int t)
{
	int maxCount = 0;
	for (auto it = database[n - 1].begin(); it != database[n - 1].end(); ++it)
	{
		if (maxCount < it->second) {
			maxCount = it->second;
		}
	}

	if (t > maxCount) {
		t = maxCount;
	}

	vector<double> Nr(maxCount + 1, double(0));

	Nr[0] = pow(vocab, n) - database[n - 1].size(); // unseen ngrams

	for (auto m = database[n - 1].begin(); m != database[n - 1].end(); ++m)
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

	for (auto k = database[n - 1].begin(); k != database[n - 1].end(); ++k) // GT algorithm
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


double calcu_prob(vector<T> sen, vector<unordered_map<vector<T>, int>> database
	, vector<unordered_map<vector<T>, double>> GTbase
	, int n, double delta, double num_tokens, double V, int model)
{
	vector<T> w_top, w_bottom;
	double c_top = 0, c_bottom = 0, p_top = 0, p_bottom = 0, probs = 0;

	if (n == 1)
	{
		for (unsigned int j = 0; j < sen.size(); j++) {
			w_top.push_back(sen[j]);

			if (model == 0)
			{
				if (GTbase[w_top.size() - 1].count(w_top) > 0)
				{
					p_top = GTbase[w_top.size() - 1].at(w_top);
				}
				else {
					p_top = GTbase[w_top.size() - 1].at({ "UNKNOWN" });
				}
			}
			else {
				c_top = getCount(database[w_top.size() - 1], w_top);
				p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));
			}
			
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

					if (model == 0)
					{
						if (GTbase[w_top.size() - 1].count(w_top) > 0)
						{
							p_top = GTbase[w_top.size() - 1].at(w_top);
						}
						else {
							p_top = GTbase[w_top.size() - 1].at({ "UNKNOWN" });
						}
					}
					else {
						c_top = getCount(database[w_top.size() - 1], w_top);
						p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));
					}
					
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

					if (model == 0) 
					{
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
					}
					else {
						c_top = getCount(database[w_top.size() - 1], w_top);
						p_top = addDelta(c_top, delta, num_tokens, pow(V, w_top.size()));

						c_bottom = getCount(database[w_bottom.size() - 1], w_bottom);
						p_bottom = addDelta(c_bottom, delta, num_tokens, pow(V, w_bottom.size()));
					}

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


void spell_check(input_sen& sen, unordered_map<T, set<T>> candidate
	, vector<unordered_map<vector<T>, int>> database
	, vector<unordered_map<vector<T>, double>> GTbase
	, int n, double delta, double num_tokens, double V, int model)
{
	vector<double> each_max_prob;
	vector<T> each_max_candi; // for max candidate in each set

	for (unsigned int p = 0; p < sen.size(); p++) // loop each word in sentence
	{
		set<T> candi_set = candidate.at(sen[p]);

		double max_prob = -INFINITY;
		T max_candi;

		for (auto w: candi_set) // loop each candidate in set
		{
			vector<T> possible_sen = sen;
			possible_sen[p] = w;

			double candi_prob = calcu_prob(possible_sen, database, GTbase, n, delta, num_tokens, V, model);

			if (candi_prob > max_prob) {
				max_prob = candi_prob;
				max_candi = w;
			}
		}

		each_max_prob.push_back(max_prob);
		each_max_candi.push_back(max_candi);
	}

	auto biggest = max_element(begin(each_max_prob), end(each_max_prob));

	int final_max_idx = distance(begin(each_max_prob), biggest); // the idx of the word that needs to be replaced
	T final_candi = each_max_candi[final_max_idx]; // the candidate for that word

	sen[final_max_idx] = final_candi;

	// print out new sentence
	for (unsigned int x = 0; x < sen.size(); x++)
	{
		cout << sen[x] << " ";
	}
}


void main(int argc, char **argv) {
	int n, t, model;
	double delta;
	string inputFile1, inputFile2, inputFile3;

	cout << "P7 ";
	cin >> inputFile1 >> inputFile2 >> inputFile3 >> n >> t >> delta >> model;

	vector<T> sentence, word_dict;

	read_tokens(inputFile3, word_dict, false);
	read_tokens(inputFile2, sentence, true);


	////////// generate candidate sets and split sentences //////////
	set<T> unique_word;

	vector<int> stop_idx;
	stop_idx.push_back(-1);

	for (unsigned int j = 0; j < sentence.size(); j++) 
	{
		vector<T> each_sen;
		if (sentence[j] != "<END>") {
			unique_word.insert(sentence[j]);
		}
		else {
			stop_idx.push_back(j);
		}
	}

	vector<vector<T>> all_sens; // split sentences
	for (unsigned int s = 0; s < stop_idx.size()-1; s++) 
	{
		vector<T> each_sen(sentence.begin() + (stop_idx[s]+1), sentence.begin() + stop_idx[s + 1]);
		
		all_sens.push_back(each_sen);
	}

	
	unordered_map<T, set<T>> candidate; // get candidate sets for all words in sentences
	for (auto w : unique_word) {
		set<T> candi;
		candi.insert(w);

		for (unsigned int k = 0; k < word_dict.size(); k++)
		{
			size_t dist = uiLevenshteinDistance(w, word_dict[k]);

			if (dist <= 1) {
				candi.insert(word_dict[k]);
			}
		}

		candidate[w] = candi;
	}

	////////// make ngram hasmaps //////////

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


	////////// decide which word to be replaced and its candidate //////////

	vector<unordered_map<vector<T>, double>> GTbase; // get GT collection

	if (model == 0) {

		for (unsigned int y = 0; y < n; y++)
		{
			unordered_map<vector<T>, double> norm_p;
			GT(norm_p, database, fileSize, V, y+1, t);

			GTbase.push_back(norm_p);
		}

		for (unsigned int z = 0; z < all_sens.size(); z++) {
			cout << "\n";
			spell_check(all_sens[z], candidate, database, GTbase, n, delta, fileSize, V, 0);
		}
	}
	else {
		for (unsigned int z = 0; z < all_sens.size(); z++) {
			cout << "\n";
			spell_check(all_sens[z], candidate, database, GTbase, n, delta, fileSize, V, 1);
		}
	}

	cout << "\n\n";

}


