Email Rodrigo if ESP interface changes

- NULL
- MakeStringException
  https://github.com/hpcc-systems/HPCC-Platform/pull/14491#discussion_r565166491
- strToBool  
- virtual ... override
  https://github.com/hpcc-systems/HPCC-Platform/pull/14440#discussion_r556406910
- return targetNames.ordinality();
- minimize the amount of conditional code if possible/clean
- use 'dirty' flag
- BoolHash validTargets; ->
std::set<std::string> validTargets;
- io.clear() or io->Release() before removing a file

inline bool strsame(const char* s, const char* t) { return (s == t) || (s && t && strcmp(s,t)==0); }  // also allow nulls
inline bool strisame(const char* s, const char* t) { return (s == t) || (s && t && stricmp(s,t)==0); }  // also allow nulls
inline bool hasPrefix(const char * text, const char * prefix, bool caseSensitive) //Is the text started with the prefix?

----------Async calls
auto func = [lfn, fileDesc](unsigned partNum)
{
 // collect and set
};

CAsyncForFunc<decltype(func)> async(func);
async.For(fileDesc->numParts(), 100);
--------------------

Using #ifndef _CONTAINERIZED for bare metal only code

Owned<IRemoteConnection> conn = querySDS().connect(path, myProcessSession(), RTM_LOCK_WRITE, daliConnectTimeoutMs);
conn->close(true); //the close(true) will delete the thing it's connected to.

Owned<IRemoteConnection> conn = querySDS().connect(path, myProcessSession(), RTM_LOCK_WRITE, daliConnectTimeoutMs);
//conn->commit();
conn->close();
a close() implies a commit(), so can just call close()


When no need to line:
replace Owned<IPropertyTree> root = conn->getRoot();
with IPropertyTree *root = conn->queryRoot();

Replace: Owned<IPropertyTree> child = root->getPropTree(tail);
         root->removeTree(child);
With:       root->removeProp(tail);


