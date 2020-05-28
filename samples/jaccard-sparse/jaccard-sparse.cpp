
// sort -R sparse_binary.tsv |head -10 > sparse_binary_query_10.tsv
// ./jaccard-sparse create -d 100 -D J sparse
// ./jaccard-sparse append sparse sparse_binary.tsv
// ./jaccard-sparse search sparse sparse_binary_query_10.tsv
//

#include	"NGT/Command.h"

using namespace std;

void help() {
  cerr << "Usage : jaccard-sparse command index [data]" << endl;
  cerr << "           command : info create search append" << endl;
}

void 
append(NGT::Args &args)
{
  const string usage = "Usage: jaccard-sparse append [-p #-of-thread] [-n data-size] "
    "index(output) [data.tsv(input)]";
  string database;
  try {
    database = args.get("#1");
  } catch (...) {
    cerr << "jaccard-sparse: Error: DB is not specified." << endl;
    cerr << usage << endl;
    return;
  }
  string data;
  try {
    data = args.get("#2");
  } catch (...) {
    cerr << "jaccard-sparse: Warning: No specified object file. Just build an index for the existing objects." << endl;
  }

  int threadSize = args.getl("p", 50);
  size_t dataSize = args.getl("n", 0);

  try {
    NGT::Index	index(database);
    ifstream	is(data);
    if (!is) {
      cerr << "Cannot open the specified data file. " << data << endl;
      return;
    }
    string line;
    size_t count = 0;
    while(getline(is, line)) {
      if (dataSize > 0 && count >= dataSize) {
	break;
      }
      count++;
      vector<uint32_t> object;
      stringstream	linestream(line);
      while (!linestream.eof()) {
	uint32_t value;
	linestream >> value;
	object.push_back(value);
      }
      NGT::ObjectID id = index.append(index.makeSparseObject(object));
    }
    index.createIndex(threadSize);
    index.saveIndex(database);
  } catch (NGT::Exception &err) {
    cerr << "jaccard-sparse: Error " << err.what() << endl;
    cerr << usage << endl;
  }
  return;
}


void
search(NGT::Index &index, NGT::Command::SearchParameter &searchParameter, ostream &stream)
{

  std::ifstream		is(searchParameter.query);
  if (!is) {
    std::cerr << "Cannot open the specified file. " << searchParameter.query << std::endl;
    return;
  }

  string line;
  double totalTime	= 0;
  size_t queryCount	= 0;

  while(getline(is, line)) {
    if (searchParameter.querySize > 0 && queryCount >= searchParameter.querySize) {
      break;
    }
    vector<uint32_t>	query;
    stringstream	linestream(line);
    while (!linestream.eof()) {
      uint32_t value;
      linestream >> value;
      query.push_back(value);
    }
    auto sparseQuery = index.makeSparseObject(query);
    queryCount++;
    NGT::SearchQuery	sc(sparseQuery);
    NGT::ObjectDistances objects;
    sc.setResults(&objects);
    sc.setSize(searchParameter.size);
    sc.setRadius(searchParameter.radius);
    if (searchParameter.accuracy > 0.0) {
      sc.setExpectedAccuracy(searchParameter.accuracy);
    } else {
      sc.setEpsilon(searchParameter.beginOfEpsilon);
    }
    sc.setEdgeSize(searchParameter.edgeSize);
    NGT::Timer timer;
    switch (searchParameter.indexType) {
    case 't': timer.start(); index.search(sc); timer.stop(); break;
    case 'g': timer.start(); index.searchUsingOnlyGraph(sc); timer.stop(); break;
    case 's': timer.start(); index.linearSearch(sc); timer.stop(); break;
    }
    totalTime += timer.time;
    stream << "Query No." << queryCount << endl;
    stream << "Rank\tID\tDistance" << endl;
    for (size_t i = 0; i < objects.size(); i++) {
      stream << i + 1 << "\t" << objects[i].id << "\t";
      stream << objects[i].distance << endl;
    }
    stream << "# End of Search" << endl;
  }
  stream << "Average Query Time= " << totalTime / (double)queryCount  << " (sec), " 
	 << totalTime * 1000.0 / (double)queryCount << " (msec), (" 
	 << totalTime << "/" << queryCount << ")" << endl;
}

void
search(NGT::Args &args) {
  const string usage = "Usage: ngt search [-i index-type(g|t|s)] [-n result-size] [-e epsilon] [-E edge-size] "
    "[-m open-mode(r|w)] [-o output-mode] index(input) query.tsv(input)";

  string database;
  try {
    database = args.get("#1");
  } catch (...) {
    cerr << "jaccard-sparse: Error: DB is not specified" << endl;
    cerr << usage << endl;
    return;
  }

  NGT::Command::SearchParameter searchParameter(args);

  try {
    NGT::Index	index(database, searchParameter.openMode == 'r');
    search(index, searchParameter, cout);
  } catch (NGT::Exception &err) {
    cerr << "jaccard-sparse: Error " << err.what() << endl;
    cerr << usage << endl;
  } catch (...) {
    cerr << "jaccard-sparse: Error" << endl;
    cerr << usage << endl;
  }

}

int
main(int argc, char **argv)
{

  NGT::Args args(argc, argv);

  NGT::Command ngt;

  string command;
  try {
    command = args.get("#0");
  } catch(...) {
    help();
    return 0;
  }

  try {
    if (command == "create") {
      ngt.create(args);
    } else if (command == "append") {
      append(args);
    } else if (command == "search") {
      search(args);
    } else {
      cerr << "jaccard-sparse: Error: Illegal command. " << command << endl;
      help();
    }
  } catch(NGT::Exception &err) {
    cerr << "jaccard-sparse: Error: " << err.what() << endl;
    help();
    return 0;
  }
  return 0;
}

