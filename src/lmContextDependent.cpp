// $Id: lmContextDependent.cpp 3686 2010-10-15 11:55:32Z bertoldi $

/******************************************************************************
 IrstLM: IRST Language Model Toolkit
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 
 ******************************************************************************/
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "lmContainer.h"
#include "lmContextDependent.h"
#include "util.h"

using namespace std;
	
inline void error(const char* message)
{
  std::cerr << message << "\n";
  throw std::runtime_error(message);
}

namespace irstlm {
lmContextDependent::lmContextDependent(float nlf, float dlf)
{
  ngramcache_load_factor = nlf;
  dictionary_load_factor = dlf;
  m_lm=NULL;
  m_topicmodel=NULL;
	
  order=0;
  memmap=0;
  isInverted=false;

}

lmContextDependent::~lmContextDependent()
{
  if (m_lm) delete m_lm;
  if (m_topicmodel) delete m_topicmodel;
}

void lmContextDependent::load(const std::string &filename,int mmap)
{
  VERBOSE(2,"lmContextDependent::load(const std::string &filename,int memmap)" << std::endl);
  VERBOSE(2," filename:|" << filename << "|" << std::endl);
	
	
  dictionary_upperbound=1000000;
  int memmap=mmap;
	
  //get info from the configuration file
  fstream inp(filename.c_str(),ios::in|ios::binary);
	
  char line[MAX_LINE];
  const char* words[LMCONFIGURE_MAX_TOKEN];
  int tokenN;
  inp.getline(line,MAX_LINE,'\n');
  tokenN = parseWords(line,words,LMCONFIGURE_MAX_TOKEN);
	
  if (tokenN != 2 || ((strcmp(words[0],"LMCONTEXTDEPENDENT") != 0) && (strcmp(words[0],"lmcontextdependent")!=0)))
    error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
	
//reading ngram-based LM
  inp.getline(line,BUFSIZ,'\n');
  tokenN = parseWords(line,words,3);

  if(tokenN < 2 || tokenN >3) {
    error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
  }

  //check whether the (textual) LM has to be loaded as inverted
  m_isinverted = false;
  if(tokenN == 3) {
    if (strcmp(words[2],"inverted") == 0)
      m_isinverted = true;
  }
  VERBOSE(2,"m_isinverted:" << m_isinverted << endl);

  m_lm_weight = (float) atof(words[0]);

  //checking the language model type
  m_lm=lmContainer::CreateLanguageModel(words[1],ngramcache_load_factor,dictionary_load_factor);

  //let know that table has inverted n-grams
  m_lm->is_inverted(m_isinverted);  //set inverted flag for each LM

  m_lm->setMaxLoadedLevel(requiredMaxlev);

  m_lm->load(words[1], memmap);
  dict=m_lm->getDict();
  getDict()->genoovcode();

  m_lm->init_caches(m_lm->maxlevel());


//reading bigram-base topic model
  inp.getline(line,BUFSIZ,'\n');
  tokenN = parseWords(line,words,3);

  if(tokenN < 2 || tokenN >3) {
    error((char*)"ERROR: wrong header format of configuration file\ncorrect format: LMCONTEXTDEPENDENT\nweight_of_ngram_LM filename_of_LM\nweight_of_topic_model filename_of_topic_model");
  }

  //loading topic model and initialization
  m_topicmodel_weight = (float) atof(words[0]);
  //m_topic_model = new  xxxxxxxxxxxxxxxx


  inp.close();
}

double lmContextDependent::lprob(ngram ng, topic_map& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
{
  double lm_prob = m_lm->clprob(ng, bow, bol, maxsuffptr, statesize, extendible);
  double topic_prob = 0.0;  // to_CHECK
  double ret_prob = m_lm_weight * lm_prob + m_topicmodel_weight * topic_prob;

  return ret_prob;
}

double lmContextDependent::lprob(int* codes, int sz, topic_map& topic_weights, double* bow,int* bol,char** maxsuffptr,unsigned int* statesize,bool* extendible)
{
  //create the actual ngram
  ngram ong(dict);
  ong.pushc(codes,sz);
  MY_ASSERT (ong.size == sz);

  return lprob(ong, topic_weights, bow, bol, maxsuffptr, statesize, extendible);	
}

double lmContextDependent::setlogOOVpenalty(int dub)
{
  MY_ASSERT(dub > dict->size());
  m_lm->setlogOOVpenalty(dub);  //set OOV Penalty by means of DUB
  double OOVpenalty = m_lm->getlogOOVpenalty();  //get OOV Penalty
  logOOVpenalty=log(OOVpenalty);
  return logOOVpenalty;
}
}//namespace irstlm
