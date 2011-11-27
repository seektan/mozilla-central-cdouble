// ----------------------------------------------------------------------------
// Win32 profiler support.

#include <windows.h>
#include "v8-support.h"
#include "platform.h"

class Sampler::PlatformData : public Malloced {
 public:
  // Get a handle to the calling thread. This is the thread that we are
  // going to profile. We need to make a copy of the handle because we are
  // going to use it in the sampler thread. Using GetThreadHandle() will
  // not work in this case. We're using OpenThread because DuplicateHandle
  // for some reason doesn't work in Chrome's sandbox.
  PlatformData() : profiled_thread_(OpenThread(THREAD_GET_CONTEXT |
                                               THREAD_SUSPEND_RESUME |
                                               THREAD_QUERY_INFORMATION,
                                               false,
                                               GetCurrentThreadId())) {}

  ~PlatformData() {
    if (profiled_thread_ != NULL) {
      CloseHandle(profiled_thread_);
      profiled_thread_ = NULL;
    }
  }

  HANDLE profiled_thread() { return profiled_thread_; }

 private:
  HANDLE profiled_thread_;
};


class SamplerThread : public Thread {
 public:
  SamplerThread(int interval, Sampler* sampler)
      : Thread("SamplerThread"),
        interval_(interval),
        sampler_(sampler) {}

  static void Start(Sampler* sampler) {
    if (instance_ == NULL) {
      instance_ = new SamplerThread(sampler->interval(), sampler);
      instance_->Start();
    } else {
      ASSERT(instance_->interval_ == sampler->interval());
    }
  }

  static void Stop() {
    delete instance_;
    instance_ = NULL;
  }

  // Implement Thread::Run().
  virtual void Run() {
    while (true) {
      SampleContext(sampler_);
      OS::Sleep(interval_);
    }
  }

  void SampleContext(Sampler* sampler) {
    HANDLE profiled_thread = sampler->platform_data()->profiled_thread();
    if (profiled_thread == NULL) return;

    // Context used for sampling the register state of the profiled thread.
    CONTEXT context;
    memset(&context, 0, sizeof(context));

    TickSample sample_obj;
    TickSample* sample = CpuProfiler::TickSampleEvent(sampler->isolate());
    if (sample == NULL) sample = &sample_obj;

    static const DWORD kSuspendFailed = static_cast<DWORD>(-1);
    if (SuspendThread(profiled_thread) == kSuspendFailed) return;
    sample->state = sampler->isolate()->current_vm_state();

    context.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(profiled_thread, &context) != 0) {
#if V8_HOST_ARCH_X64
      sample->pc = reinterpret_cast<Address>(context.Rip);
      sample->sp = reinterpret_cast<Address>(context.Rsp);
      sample->fp = reinterpret_cast<Address>(context.Rbp);
#else
      sample->pc = reinterpret_cast<Address>(context.Eip);
      sample->sp = reinterpret_cast<Address>(context.Esp);
      sample->fp = reinterpret_cast<Address>(context.Ebp);
#endif
      sampler->SampleStack(sample);
      sampler->Tick(sample);
    }
    ResumeThread(profiled_thread);
  }

  Sampler* sampler_;
  const int interval_;

  // Protects the process wide state below.
  static Mutex* mutex_;
  static SamplerThread* instance_;

  DISALLOW_COPY_AND_ASSIGN(SamplerThread);
};


Mutex* SamplerThread::mutex_ = OS::CreateMutex();
SamplerThread* SamplerThread::instance_ = NULL;


Sampler::Sampler(Isolate* isolate, int interval)
    : isolate_(isolate),
      interval_(interval),
      profiling_(false),
      active_(false),
      samples_taken_(0) {
  data_ = new PlatformData;
}


Sampler::~Sampler() {
  ASSERT(!IsActive());
  delete data_;
}


void Sampler::Start() {
  ASSERT(!IsActive());
  SetActive(true);
  SamplerThread::Start(this);
}


void Sampler::Stop() {
  ASSERT(IsActive());
  SamplerThread::Stop();
  SetActive(false);
}



