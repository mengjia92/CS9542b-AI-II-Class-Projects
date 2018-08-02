//  compute the counts of words in an input file and output k most frequent words together with their counts

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include<algorithm>

using namespace std;

#include "fileRead.h"
#include "VectorHash.h"

typedef string T; // for string language model

void main()
{
	int k;
	int N = 0;
	string inputFile;

	cout << "P1 ";
	cin >> inputFile >> k;

	try
	{
		vector<T> words;
		unordered_map<string, int> wordsdict;

		read_tokens(inputFile, words, false); // reads strings without EOS 

		N = words.size();

		for (int i = 0; i < N; i++) // build words dictionary
		{
			if (wordsdict.count(words[i]) == 0)
				wordsdict[words[i]] = 1;
			else
				wordsdict[words[i]] += 1;
		}

		vector<pair<string, int>> W; // dump words dict to vector of pairs

		copy(wordsdict.begin(), wordsdict.end(), back_inserter(W));

		sort(W.begin(), W.end(), [](auto &left, auto &right) {
			return left.second > right.second;
		}); // sort pairs by descending order


		double freqSum = 0;

		for (unsigned int j = 0; j < k; j++)
		{
			freqSum += W[j].second;
			cout << "\n" << W[j].first << ", " << W[j].second;
		}
		cout << "\n" << freqSum / N * 100 << "%" << "\n" ;
	}
		
	
	catch (FileReadException e)
	{
		e.Report();
	}

}
