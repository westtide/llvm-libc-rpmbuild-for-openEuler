//===-- PerfReader.h - perfscript reader -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_PROFGEN_PERFREADER_H
#define LLVM_TOOLS_LLVM_PROFGEN_PERFREADER_H
#include "ErrorHandling.h"
#include "ProfiledBinary.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Regex.h"
#include <cstdint>
#include <fstream>
#include <map>

using namespace llvm;
using namespace sampleprof;

namespace llvm {

class CleanupInstaller;

namespace sampleprof {

// Stream based trace line iterator
class TraceStream {
  std::string CurrentLine;
  std::ifstream Fin;
  bool IsAtEoF = false;
  uint64_t LineNumber = 0;

public:
  TraceStream(StringRef Filename) : Fin(Filename.str()) {
    if (!Fin.good())
      exitWithError("Error read input perf script file", Filename);
    advance();
  }

  StringRef getCurrentLine() {
    assert(!IsAtEoF && "Line iterator reaches the End-of-File!");
    return CurrentLine;
  }

  uint64_t getLineNumber() { return LineNumber; }

  bool isAtEoF() { return IsAtEoF; }

  // Read the next line
  void advance() {
    if (!std::getline(Fin, CurrentLine)) {
      IsAtEoF = true;
      return;
    }
    LineNumber++;
  }
};

// The type of input format.
enum PerfFormat {
  UnknownFormat = 0,
  PerfData = 1,            // Raw linux perf.data.
  PerfScript = 2,          // Perf script create by `perf script` command.
  UnsymbolizedProfile = 3, // Unsymbolized profile generated by llvm-profgen.

};

// The type of perfscript content.
enum PerfContent {
  UnknownContent = 0,
  LBR = 1,      // Only LBR sample.
  LBRStack = 2, // Hybrid sample including call stack and LBR stack.
};

struct PerfInputFile {
  std::string InputFile;
  PerfFormat Format = PerfFormat::UnknownFormat;
  PerfContent Content = PerfContent::UnknownContent;
};

// The parsed LBR sample entry.
struct LBREntry {
  uint64_t Source = 0;
  uint64_t Target = 0;
  LBREntry(uint64_t S, uint64_t T) : Source(S), Target(T) {}

#ifndef NDEBUG
  void print() const {
    dbgs() << "from " << format("%#010x", Source) << " to "
           << format("%#010x", Target);
  }
#endif
};

#ifndef NDEBUG
static inline void printLBRStack(const SmallVectorImpl<LBREntry> &LBRStack) {
  for (size_t I = 0; I < LBRStack.size(); I++) {
    dbgs() << "[" << I << "] ";
    LBRStack[I].print();
    dbgs() << "\n";
  }
}

static inline void printCallStack(const SmallVectorImpl<uint64_t> &CallStack) {
  for (size_t I = 0; I < CallStack.size(); I++) {
    dbgs() << "[" << I << "] " << format("%#010x", CallStack[I]) << "\n";
  }
}
#endif

// Hash interface for generic data of type T
// Data should implement a \fn getHashCode and a \fn isEqual
// Currently getHashCode is non-virtual to avoid the overhead of calling vtable,
// i.e we explicitly calculate hash of derived class, assign to base class's
// HashCode. This also provides the flexibility for calculating the hash code
// incrementally(like rolling hash) during frame stack unwinding since unwinding
// only changes the leaf of frame stack. \fn isEqual is a virtual function,
// which will have perf overhead. In the future, if we redesign a better hash
// function, then we can just skip this or switch to non-virtual function(like
// just ignore comparison if hash conflicts probabilities is low)
template <class T> class Hashable {
public:
  std::shared_ptr<T> Data;
  Hashable(const std::shared_ptr<T> &D) : Data(D) {}

  // Hash code generation
  struct Hash {
    uint64_t operator()(const Hashable<T> &Key) const {
      // Don't make it virtual for getHashCode
      uint64_t Hash = Key.Data->getHashCode();
      assert(Hash && "Should generate HashCode for it!");
      return Hash;
    }
  };

  // Hash equal
  struct Equal {
    bool operator()(const Hashable<T> &LHS, const Hashable<T> &RHS) const {
      // Precisely compare the data, vtable will have overhead.
      return LHS.Data->isEqual(RHS.Data.get());
    }
  };

  T *getPtr() const { return Data.get(); }
};

struct PerfSample {
  // LBR stack recorded in FIFO order.
  SmallVector<LBREntry, 16> LBRStack;
  // Call stack recorded in FILO(leaf to root) order, it's used for CS-profile
  // generation
  SmallVector<uint64_t, 16> CallStack;

  virtual ~PerfSample() = default;
  uint64_t getHashCode() const {
    // Use simple DJB2 hash
    auto HashCombine = [](uint64_t H, uint64_t V) {
      return ((H << 5) + H) + V;
    };
    uint64_t Hash = 5381;
    for (const auto &Value : CallStack) {
      Hash = HashCombine(Hash, Value);
    }
    for (const auto &Entry : LBRStack) {
      Hash = HashCombine(Hash, Entry.Source);
      Hash = HashCombine(Hash, Entry.Target);
    }
    return Hash;
  }

  bool isEqual(const PerfSample *Other) const {
    const SmallVector<uint64_t, 16> &OtherCallStack = Other->CallStack;
    const SmallVector<LBREntry, 16> &OtherLBRStack = Other->LBRStack;

    if (CallStack.size() != OtherCallStack.size() ||
        LBRStack.size() != OtherLBRStack.size())
      return false;

    if (!std::equal(CallStack.begin(), CallStack.end(), OtherCallStack.begin()))
      return false;

    for (size_t I = 0; I < OtherLBRStack.size(); I++) {
      if (LBRStack[I].Source != OtherLBRStack[I].Source ||
          LBRStack[I].Target != OtherLBRStack[I].Target)
        return false;
    }
    return true;
  }

#ifndef NDEBUG
  uint64_t Linenum = 0;

  void print() const {
    dbgs() << "Line " << Linenum << "\n";
    dbgs() << "LBR stack\n";
    printLBRStack(LBRStack);
    dbgs() << "Call stack\n";
    printCallStack(CallStack);
  }
#endif
};
// After parsing the sample, we record the samples by aggregating them
// into this counter. The key stores the sample data and the value is
// the sample repeat times.
using AggregatedCounter =
    std::unordered_map<Hashable<PerfSample>, uint64_t,
                       Hashable<PerfSample>::Hash, Hashable<PerfSample>::Equal>;

using SampleVector = SmallVector<std::tuple<uint64_t, uint64_t, uint64_t>, 16>;

inline bool isValidFallThroughRange(uint64_t Start, uint64_t End,
                                    ProfiledBinary *Binary) {
  // Start bigger than End is considered invalid.
  // LBR ranges cross the unconditional jmp are also assumed invalid.
  // It's found that perf data may contain duplicate LBR entries that could form
  // a range that does not reflect real execution flow on some Intel targets,
  // e.g. Skylake. Such ranges are ususally very long. Exclude them since there
  // cannot be a linear execution range that spans over unconditional jmp.
  return Start <= End && !Binary->rangeCrossUncondBranch(Start, End);
}

// The state for the unwinder, it doesn't hold the data but only keep the
// pointer/index of the data, While unwinding, the CallStack is changed
// dynamicially and will be recorded as the context of the sample
struct UnwindState {
  // Profiled binary that current frame address belongs to
  const ProfiledBinary *Binary;
  // Call stack trie node
  struct ProfiledFrame {
    const uint64_t Address = DummyRoot;
    ProfiledFrame *Parent;
    SampleVector RangeSamples;
    SampleVector BranchSamples;
    std::unordered_map<uint64_t, std::unique_ptr<ProfiledFrame>> Children;

    ProfiledFrame(uint64_t Addr = 0, ProfiledFrame *P = nullptr)
        : Address(Addr), Parent(P) {}
    ProfiledFrame *getOrCreateChildFrame(uint64_t Address) {
      assert(Address && "Address can't be zero!");
      auto Ret = Children.emplace(
          Address, std::make_unique<ProfiledFrame>(Address, this));
      return Ret.first->second.get();
    }
    void recordRangeCount(uint64_t Start, uint64_t End, uint64_t Count) {
      RangeSamples.emplace_back(std::make_tuple(Start, End, Count));
    }
    void recordBranchCount(uint64_t Source, uint64_t Target, uint64_t Count) {
      BranchSamples.emplace_back(std::make_tuple(Source, Target, Count));
    }
    bool isDummyRoot() { return Address == DummyRoot; }
    bool isExternalFrame() { return Address == ExternalAddr; }
    bool isLeafFrame() { return Children.empty(); }
  };

  ProfiledFrame DummyTrieRoot;
  ProfiledFrame *CurrentLeafFrame;
  // Used to fall through the LBR stack
  uint32_t LBRIndex = 0;
  // Reference to PerfSample.LBRStack
  const SmallVector<LBREntry, 16> &LBRStack;
  // Used to iterate the address range
  InstructionPointer InstPtr;
  // Indicate whether unwinding is currently in a bad state which requires to
  // skip all subsequent unwinding.
  bool Invalid = false;
  UnwindState(const PerfSample *Sample, const ProfiledBinary *Binary)
      : Binary(Binary), LBRStack(Sample->LBRStack),
        InstPtr(Binary, Sample->CallStack.front()) {
    initFrameTrie(Sample->CallStack);
  }

  bool validateInitialState() {
    uint64_t LBRLeaf = LBRStack[LBRIndex].Target;
    uint64_t LeafAddr = CurrentLeafFrame->Address;
    assert((LBRLeaf != ExternalAddr || LBRLeaf == LeafAddr) &&
           "External leading LBR should match the leaf frame.");

    // When we take a stack sample, ideally the sampling distance between the
    // leaf IP of stack and the last LBR target shouldn't be very large.
    // Use a heuristic size (0x100) to filter out broken records.
    if (LeafAddr < LBRLeaf || LeafAddr - LBRLeaf >= 0x100) {
      WithColor::warning() << "Bogus trace: stack tip = "
                           << format("%#010x", LeafAddr)
                           << ", LBR tip = " << format("%#010x\n", LBRLeaf);
      return false;
    }
    return true;
  }

  void checkStateConsistency() {
    assert(InstPtr.Address == CurrentLeafFrame->Address &&
           "IP should align with context leaf");
  }

  void setInvalid() { Invalid = true; }
  bool hasNextLBR() const { return LBRIndex < LBRStack.size(); }
  uint64_t getCurrentLBRSource() const { return LBRStack[LBRIndex].Source; }
  uint64_t getCurrentLBRTarget() const { return LBRStack[LBRIndex].Target; }
  const LBREntry &getCurrentLBR() const { return LBRStack[LBRIndex]; }
  bool IsLastLBR() const { return LBRIndex == 0; }
  bool getLBRStackSize() const { return LBRStack.size(); }
  void advanceLBR() { LBRIndex++; }
  ProfiledFrame *getParentFrame() { return CurrentLeafFrame->Parent; }

  void pushFrame(uint64_t Address) {
    CurrentLeafFrame = CurrentLeafFrame->getOrCreateChildFrame(Address);
  }

  void switchToFrame(uint64_t Address) {
    if (CurrentLeafFrame->Address == Address)
      return;
    CurrentLeafFrame = CurrentLeafFrame->Parent->getOrCreateChildFrame(Address);
  }

  void popFrame() { CurrentLeafFrame = CurrentLeafFrame->Parent; }

  void clearCallStack() { CurrentLeafFrame = &DummyTrieRoot; }

  void initFrameTrie(const SmallVectorImpl<uint64_t> &CallStack) {
    ProfiledFrame *Cur = &DummyTrieRoot;
    for (auto Address : reverse(CallStack)) {
      Cur = Cur->getOrCreateChildFrame(Address);
    }
    CurrentLeafFrame = Cur;
  }

  ProfiledFrame *getDummyRootPtr() { return &DummyTrieRoot; }
};

// Base class for sample counter key with context
struct ContextKey {
  uint64_t HashCode = 0;
  virtual ~ContextKey() = default;
  uint64_t getHashCode() {
    if (HashCode == 0)
      genHashCode();
    return HashCode;
  }
  virtual void genHashCode() = 0;
  virtual bool isEqual(const ContextKey *K) const {
    return HashCode == K->HashCode;
  };

  // Utilities for LLVM-style RTTI
  enum ContextKind { CK_StringBased, CK_AddrBased };
  const ContextKind Kind;
  ContextKind getKind() const { return Kind; }
  ContextKey(ContextKind K) : Kind(K){};
};

// String based context id
struct StringBasedCtxKey : public ContextKey {
  SampleContextFrameVector Context;

  bool WasLeafInlined;
  StringBasedCtxKey() : ContextKey(CK_StringBased), WasLeafInlined(false){};
  static bool classof(const ContextKey *K) {
    return K->getKind() == CK_StringBased;
  }

  bool isEqual(const ContextKey *K) const override {
    const StringBasedCtxKey *Other = dyn_cast<StringBasedCtxKey>(K);
    return Context == Other->Context;
  }

  void genHashCode() override {
    HashCode = hash_value(SampleContextFrames(Context));
  }
};

// Address-based context id
struct AddrBasedCtxKey : public ContextKey {
  SmallVector<uint64_t, 16> Context;

  bool WasLeafInlined;
  AddrBasedCtxKey() : ContextKey(CK_AddrBased), WasLeafInlined(false){};
  static bool classof(const ContextKey *K) {
    return K->getKind() == CK_AddrBased;
  }

  bool isEqual(const ContextKey *K) const override {
    const AddrBasedCtxKey *Other = dyn_cast<AddrBasedCtxKey>(K);
    return Context == Other->Context;
  }

  void genHashCode() override {
    HashCode = hash_combine_range(Context.begin(), Context.end());
  }
};

// The counter of branch samples for one function indexed by the branch,
// which is represented as the source and target offset pair.
using BranchSample = std::map<std::pair<uint64_t, uint64_t>, uint64_t>;
// The counter of range samples for one function indexed by the range,
// which is represented as the start and end offset pair.
using RangeSample = std::map<std::pair<uint64_t, uint64_t>, uint64_t>;
// Wrapper for sample counters including range counter and branch counter
struct SampleCounter {
  RangeSample RangeCounter;
  BranchSample BranchCounter;

  void recordRangeCount(uint64_t Start, uint64_t End, uint64_t Repeat) {
    assert(Start <= End && "Invalid instruction range");
    RangeCounter[{Start, End}] += Repeat;
  }
  void recordBranchCount(uint64_t Source, uint64_t Target, uint64_t Repeat) {
    BranchCounter[{Source, Target}] += Repeat;
  }
};

// Sample counter with context to support context-sensitive profile
using ContextSampleCounterMap =
    std::unordered_map<Hashable<ContextKey>, SampleCounter,
                       Hashable<ContextKey>::Hash, Hashable<ContextKey>::Equal>;

struct FrameStack {
  SmallVector<uint64_t, 16> Stack;
  ProfiledBinary *Binary;
  FrameStack(ProfiledBinary *B) : Binary(B) {}
  bool pushFrame(UnwindState::ProfiledFrame *Cur) {
    assert(!Cur->isExternalFrame() &&
           "External frame's not expected for context stack.");
    Stack.push_back(Cur->Address);
    return true;
  }

  void popFrame() {
    if (!Stack.empty())
      Stack.pop_back();
  }
  std::shared_ptr<StringBasedCtxKey> getContextKey();
};

struct AddressStack {
  SmallVector<uint64_t, 16> Stack;
  ProfiledBinary *Binary;
  AddressStack(ProfiledBinary *B) : Binary(B) {}
  bool pushFrame(UnwindState::ProfiledFrame *Cur) {
    assert(!Cur->isExternalFrame() &&
           "External frame's not expected for context stack.");
    Stack.push_back(Cur->Address);
    return true;
  }

  void popFrame() {
    if (!Stack.empty())
      Stack.pop_back();
  }
  std::shared_ptr<AddrBasedCtxKey> getContextKey();
};

/*
As in hybrid sample we have a group of LBRs and the most recent sampling call
stack, we can walk through those LBRs to infer more call stacks which would be
used as context for profile. VirtualUnwinder is the class to do the call stack
unwinding based on LBR state. Two types of unwinding are processd here:
1) LBR unwinding and 2) linear range unwinding.
Specifically, for each LBR entry(can be classified into call, return, regular
branch), LBR unwinding will replay the operation by pushing, popping or
switching leaf frame towards the call stack and since the initial call stack
is most recently sampled, the replay should be in anti-execution order, i.e. for
the regular case, pop the call stack when LBR is call, push frame on call stack
when LBR is return. After each LBR processed, it also needs to align with the
next LBR by going through instructions from previous LBR's target to current
LBR's source, which is the linear unwinding. As instruction from linear range
can come from different function by inlining, linear unwinding will do the range
splitting and record counters by the range with same inline context. Over those
unwinding process we will record each call stack as context id and LBR/linear
range as sample counter for further CS profile generation.
*/
class VirtualUnwinder {
public:
  VirtualUnwinder(ContextSampleCounterMap *Counter, ProfiledBinary *B)
      : CtxCounterMap(Counter), Binary(B) {}
  bool unwind(const PerfSample *Sample, uint64_t Repeat);
  std::set<uint64_t> &getUntrackedCallsites() { return UntrackedCallsites; }

  uint64_t NumTotalBranches = 0;
  uint64_t NumExtCallBranch = 0;
  uint64_t NumMissingExternalFrame = 0;
  uint64_t NumMismatchedProEpiBranch = 0;
  uint64_t NumMismatchedExtCallBranch = 0;
  uint64_t NumUnpairedExtAddr = 0;
  uint64_t NumPairedExtAddr = 0;

private:
  bool isSourceExternal(UnwindState &State) const {
    return State.getCurrentLBRSource() == ExternalAddr;
  }

  bool isTargetExternal(UnwindState &State) const {
    return State.getCurrentLBRTarget() == ExternalAddr;
  }

  // Determine whether the return source is from external code by checking if
  // the target's the next inst is a call inst.
  bool isReturnFromExternal(UnwindState &State) const {
    return isSourceExternal(State) &&
           (Binary->getCallAddrFromFrameAddr(State.getCurrentLBRTarget()) != 0);
  }

  // If the source is external address but it's not the `return` case, treat it
  // as a call from external.
  bool isCallFromExternal(UnwindState &State) const {
    return isSourceExternal(State) &&
           Binary->getCallAddrFromFrameAddr(State.getCurrentLBRTarget()) == 0;
  }

  bool isCallState(UnwindState &State) const {
    // The tail call frame is always missing here in stack sample, we will
    // use a specific tail call tracker to infer it.
    if (!isValidState(State))
      return false;

    if (Binary->addressIsCall(State.getCurrentLBRSource()))
      return true;

    return isCallFromExternal(State);
  }

  bool isReturnState(UnwindState &State) const {
    if (!isValidState(State))
      return false;

    // Simply check addressIsReturn, as ret is always reliable, both for
    // regular call and tail call.
    if (Binary->addressIsReturn(State.getCurrentLBRSource()))
      return true;

    return isReturnFromExternal(State);
  }

  bool isValidState(UnwindState &State) const { return !State.Invalid; }

  void unwindCall(UnwindState &State);
  void unwindLinear(UnwindState &State, uint64_t Repeat);
  void unwindReturn(UnwindState &State);
  void unwindBranch(UnwindState &State);

  template <typename T>
  void collectSamplesFromFrame(UnwindState::ProfiledFrame *Cur, T &Stack);
  // Collect each samples on trie node by DFS traversal
  template <typename T>
  void collectSamplesFromFrameTrie(UnwindState::ProfiledFrame *Cur, T &Stack);
  void collectSamplesFromFrameTrie(UnwindState::ProfiledFrame *Cur);

  void recordRangeCount(uint64_t Start, uint64_t End, UnwindState &State,
                        uint64_t Repeat);
  void recordBranchCount(const LBREntry &Branch, UnwindState &State,
                         uint64_t Repeat);

  ContextSampleCounterMap *CtxCounterMap;
  // Profiled binary that current frame address belongs to
  ProfiledBinary *Binary;
  // Keep track of all untracked callsites
  std::set<uint64_t> UntrackedCallsites;
};

// Read perf trace to parse the events and samples.
class PerfReaderBase {
public:
  PerfReaderBase(ProfiledBinary *B, StringRef PerfTrace)
      : Binary(B), PerfTraceFile(PerfTrace) {
    // Initialize the base address to preferred address.
    Binary->setBaseAddress(Binary->getPreferredBaseAddress());
  };
  virtual ~PerfReaderBase() = default;
  static std::unique_ptr<PerfReaderBase>
  create(ProfiledBinary *Binary, PerfInputFile &PerfInput,
         std::optional<int32_t> PIDFilter);

  // Entry of the reader to parse multiple perf traces
  virtual void parsePerfTraces() = 0;
  const ContextSampleCounterMap &getSampleCounters() const {
    return SampleCounters;
  }
  bool profileIsCS() { return ProfileIsCS; }

protected:
  ProfiledBinary *Binary = nullptr;
  StringRef PerfTraceFile;

  ContextSampleCounterMap SampleCounters;
  bool ProfileIsCS = false;

  uint64_t NumTotalSample = 0;
  uint64_t NumLeafExternalFrame = 0;
  uint64_t NumLeadingOutgoingLBR = 0;
};

// Read perf script to parse the events and samples.
class PerfScriptReader : public PerfReaderBase {
public:
  PerfScriptReader(ProfiledBinary *B, StringRef PerfTrace,
                   std::optional<int32_t> PID)
      : PerfReaderBase(B, PerfTrace), PIDFilter(PID) {};

  // Entry of the reader to parse multiple perf traces
  void parsePerfTraces() override;
  // Generate perf script from perf data
  static PerfInputFile convertPerfDataToTrace(ProfiledBinary *Binary,
                                              bool SkipPID, PerfInputFile &File,
                                              std::optional<int32_t> PIDFilter);
  // Extract perf script type by peaking at the input
  static PerfContent checkPerfScriptType(StringRef FileName);

  // Cleanup installers for temporary files created by perf script command.
  // Those files will be automatically removed when running destructor or
  // receiving signals.
  static SmallVector<CleanupInstaller, 2> TempFileCleanups;

protected:
  // The parsed MMap event
  struct MMapEvent {
    int64_t PID = 0;
    uint64_t Address = 0;
    uint64_t Size = 0;
    uint64_t Offset = 0;
    StringRef BinaryPath;
  };

  // Check whether a given line is LBR sample
  static bool isLBRSample(StringRef Line);
  // Check whether a given line is MMAP event
  static bool isMMapEvent(StringRef Line);
  // Parse a single line of a PERF_RECORD_MMAP event looking for a
  // mapping between the binary name and its memory layout.
  static bool extractMMapEventForBinary(ProfiledBinary *Binary, StringRef Line,
                                        MMapEvent &MMap);
  // Update base address based on mmap events
  void updateBinaryAddress(const MMapEvent &Event);
  // Parse mmap event and update binary address
  void parseMMapEvent(TraceStream &TraceIt);
  // Parse perf events/samples and do aggregation
  void parseAndAggregateTrace();
  // Parse either an MMAP event or a perf sample
  void parseEventOrSample(TraceStream &TraceIt);
  // Warn if the relevant mmap event is missing.
  void warnIfMissingMMap();
  // Emit accumulate warnings.
  void warnTruncatedStack();
  // Warn if range is invalid.
  void warnInvalidRange();
  // Extract call stack from the perf trace lines
  bool extractCallstack(TraceStream &TraceIt,
                        SmallVectorImpl<uint64_t> &CallStack);
  // Extract LBR stack from one perf trace line
  bool extractLBRStack(TraceStream &TraceIt,
                       SmallVectorImpl<LBREntry> &LBRStack);
  uint64_t parseAggregatedCount(TraceStream &TraceIt);
  // Parse one sample from multiple perf lines, override this for different
  // sample type
  void parseSample(TraceStream &TraceIt);
  // An aggregated count is given to indicate how many times the sample is
  // repeated.
  virtual void parseSample(TraceStream &TraceIt, uint64_t Count){};
  void computeCounterFromLBR(const PerfSample *Sample, uint64_t Repeat);
  // Post process the profile after trace aggregation, we will do simple range
  // overlap computation for AutoFDO, or unwind for CSSPGO(hybrid sample).
  virtual void generateUnsymbolizedProfile();
  void writeUnsymbolizedProfile(StringRef Filename);
  void writeUnsymbolizedProfile(raw_fd_ostream &OS);

  // Samples with the repeating time generated by the perf reader
  AggregatedCounter AggregatedSamples;
  // Keep track of all invalid return addresses
  std::set<uint64_t> InvalidReturnAddresses;
  // PID for the process of interest
  std::optional<int32_t> PIDFilter;
};

/*
  The reader of LBR only perf script.
  A typical LBR sample is like:
    40062f 0x4005c8/0x4005dc/P/-/-/0   0x40062f/0x4005b0/P/-/-/0 ...
          ... 0x4005c8/0x4005dc/P/-/-/0
*/
class LBRPerfReader : public PerfScriptReader {
public:
  LBRPerfReader(ProfiledBinary *Binary, StringRef PerfTrace,
                std::optional<int32_t> PID)
      : PerfScriptReader(Binary, PerfTrace, PID) {};
  // Parse the LBR only sample.
  void parseSample(TraceStream &TraceIt, uint64_t Count) override;
};

/*
  Hybrid perf script includes a group of hybrid samples(LBRs + call stack),
  which is used to generate CS profile. An example of hybrid sample:
    4005dc    # call stack leaf
    400634
    400684    # call stack root
    0x4005c8/0x4005dc/P/-/-/0   0x40062f/0x4005b0/P/-/-/0 ...
          ... 0x4005c8/0x4005dc/P/-/-/0    # LBR Entries
*/
class HybridPerfReader : public PerfScriptReader {
public:
  HybridPerfReader(ProfiledBinary *Binary, StringRef PerfTrace,
                   std::optional<int32_t> PID)
      : PerfScriptReader(Binary, PerfTrace, PID) {};
  // Parse the hybrid sample including the call and LBR line
  void parseSample(TraceStream &TraceIt, uint64_t Count) override;
  void generateUnsymbolizedProfile() override;

private:
  // Unwind the hybrid samples after aggregration
  void unwindSamples();
};

/*
   Format of unsymbolized profile:

    [frame1 @ frame2 @ ...]  # If it's a CS profile
      number of entries in RangeCounter
      from_1-to_1:count_1
      from_2-to_2:count_2
      ......
      from_n-to_n:count_n
      number of entries in BranchCounter
      src_1->dst_1:count_1
      src_2->dst_2:count_2
      ......
      src_n->dst_n:count_n
    [frame1 @ frame2 @ ...]  # Next context
      ......

Note that non-CS profile doesn't have the empty `[]` context.
*/
class UnsymbolizedProfileReader : public PerfReaderBase {
public:
  UnsymbolizedProfileReader(ProfiledBinary *Binary, StringRef PerfTrace)
      : PerfReaderBase(Binary, PerfTrace){};
  void parsePerfTraces() override;

private:
  void readSampleCounters(TraceStream &TraceIt, SampleCounter &SCounters);
  void readUnsymbolizedProfile(StringRef Filename);

  std::unordered_set<std::string> ContextStrSet;
};

} // end namespace sampleprof
} // end namespace llvm

#endif
