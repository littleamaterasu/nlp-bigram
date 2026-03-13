#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <random>
#include <cmath>

using namespace std;

class BigramLM {

private:

    unordered_map<string,int> unigram;
    unordered_map<string, unordered_map<string,int>> bigram;

    vector<string> vocabulary;

public:

    void clear()
    {
        unigram.clear();
        bigram.clear();
        vocabulary.clear();
    }

    vector<string> tokenize(string sentence)
    {
        vector<string> tokens;

        tokens.push_back("<s>");

        stringstream ss(sentence);
        string word;

        while(ss >> word)
            tokens.push_back(word);

        tokens.push_back("</s>");

        return tokens;
    }

    void buildModel(string corpusPath)
    {
        clear();

        ifstream file(corpusPath);
        string line;

        while(getline(file,line))
        {
            vector<string> tokens = tokenize(line);

            for(auto &w : tokens)
                unigram[w]++;

            for(int i=0;i<tokens.size()-1;i++)
                bigram[tokens[i]][tokens[i+1]]++;
        }

        for(auto &p : unigram)
            vocabulary.push_back(p.first);

        cout<<"Model built.\n";
        cout<<"Vocabulary size: "<<vocabulary.size()<<endl;
    }

    void saveModel(string path)
    {
        ofstream out(path);

        out<<unigram.size()<<endl;

        for(auto &p : unigram)
            out<<p.first<<" "<<p.second<<endl;

        int count=0;

        for(auto &p : bigram)
            count += p.second.size();

        out<<count<<endl;

        for(auto &p : bigram)
            for(auto &q : p.second)
                out<<p.first<<" "<<q.first<<" "<<q.second<<endl;

        cout<<"Model saved.\n";
    }

    void loadModel(string path)
    {
        clear();

        ifstream in(path);

        if(!in.is_open())
        {
            cout<<"Model file not found.\n";
            return;
        }

        int uniSize;
        in>>uniSize;

        for(int i=0;i<uniSize;i++)
        {
            string word;
            int count;

            in>>word>>count;

            unigram[word]=count;
            vocabulary.push_back(word);
        }

        int biSize;
        in>>biSize;

        for(int i=0;i<biSize;i++)
        {
            string w1,w2;
            int count;

            in>>w1>>w2>>count;

            bigram[w1][w2]=count;
        }

        cout<<"Model loaded.\n";
    }

    double bigramProb(string w1,string w2)
    {
        int V = vocabulary.size();

        int c1 = unigram[w1];
        int c12 = bigram[w1][w2];

        return (c12 + 1.0) / (c1 + V);
    }

    double sentenceProbability(string sentence)
    {
        vector<string> tokens = tokenize(sentence);

        double logProb = 0.0;

        for(int i=0;i<tokens.size()-1;i++)
        {
            double p = bigramProb(tokens[i],tokens[i+1]);

            logProb += log(p);
        }

        return exp(logProb);
    }

    string generateSentence(int maxLen=20)
    {
        random_device rd;
        mt19937 gen(rd());

        string current = "<s>";

        string sentence;

        for(int i=0;i<maxLen;i++)
        {
            if(bigram[current].empty())
                break;

            vector<string> words;
            vector<int> weights;

            for(auto &p : bigram[current])
            {
                words.push_back(p.first);
                weights.push_back(p.second);
            }

            discrete_distribution<> dist(weights.begin(),weights.end());

            string next = words[dist(gen)];

            if(next=="</s>")
                break;

            sentence += next + " ";

            current = next;
        }

        return sentence;
    }
};

int main()
{
    BigramLM model;

    // ====== ÐI?N ÐU?NG D?N ======

    string corpusPath = "corpus.txt";
    string modelPath  = "bigram_model.txt";

    // ============================

    model.loadModel(modelPath);

    while(true)
    {
        cout<<"\n===== BIGRAM LANGUAGE MODEL =====\n";
        cout<<"1. Rebuild model\n";
        cout<<"2. Tinh xac suat cau\n";
        cout<<"3. Sinh cau\n";
        cout<<"0. Thoat\n";

        int choice;
        cin>>choice;
        cin.ignore();

        if(choice==0)
            break;

        if(choice==1)
        {
            model.buildModel(corpusPath);
            model.saveModel(modelPath);
        }

        if(choice==2)
        {
            string sentence;

            cout<<"Nhap cau: ";
            getline(cin,sentence);

            double p = model.sentenceProbability(sentence);

            cout<<"Probability = "<<p<<endl;
        }

        if(choice==3)
        {
            cout<<"Generated sentence:\n";
            cout<<model.generateSentence()<<endl;
        }
    }

    return 0;
}
