#pragma once
#include "common.hpp"
#include "services/files-service/file-indexer/file-indexer-db.hpp"
#include "services/files-service/file-indexer/writer-worker.hpp"
#include <queue>

class IndexerScanner : public NonCopyable {
public:
  struct EnqueuedScan {
    FileIndexerDatabase::ScanType type;
    std::filesystem::path path;
    std::optional<size_t> maxDepth;
  };

private:
  static constexpr size_t INDEX_BATCH_SIZE = 10'000;
  static constexpr size_t MAX_PENDING_BATCH_COUNT = 10;
  static constexpr size_t BACKPRESSURE_WAIT_MS = 100;
  std::unique_ptr<FileIndexerDatabase> m_db;

  std::atomic<bool> m_alive = true;
  std::deque<std::vector<std::filesystem::path>> m_writeBatches;
  std::mutex m_batchMutex;
  std::condition_variable m_batchCv;

  std::queue<EnqueuedScan> m_scanPaths;
  std::mutex m_scanMutex;
  std::condition_variable m_scanCv;

  std::unique_ptr<WriterWorker> m_writerWorker;
  std::thread m_writerThread;

  void scan(const std::filesystem::path &path);
  void enqueueBatch(const std::vector<std::filesystem::path> &paths);

public:
  void enqueueFull(const std::filesystem::path &path);
  void enqueue(const std::filesystem::path &path,
               FileIndexerDatabase::ScanType type = FileIndexerDatabase::ScanType::Incremental,
               std::optional<size_t> maxDepth = std::nullopt);
  void run();
  void stop();
};
