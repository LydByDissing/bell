// Copyright (c) Kuba Szczodrzyński 2022-1-7.

#pragma once

#include "ByteStream.h"
#include "Task.h"
#include "WrappedSemaphore.h"
#include <atomic>
#include <memory>
#include <mutex>

/**
 * This class implements a wrapper around an arbitrary bell::ByteStream,
 * providing a circular reading buffer with configurable thresholds.
 *
 * The BufferedStream runs a bell::Task when it's started, so the caller can
 * access the buffer's data asynchronously, whenever needed. The buffer is refilled
 * automatically from source stream.
 *
 * The class implements bell::ByteStream's methods, although for proper functioning,
 * the caller code should be modified to check isReady() and isNotReady() flags.
 *
 * If the actual reading code can't be modified, waitForReady allows to wait for buffer readiness
 * during reading. Keep in mind that using the semaphore is probably more resource effective.
 * In this case, endWithSource should be enabled, otherwise the read() will block forever when
 * the source stream ends.
 */
class BufferedStream : public bell::ByteStream, bell::Task {
  public:
	/**
	 * @param taskName name to use for the reading task
	 * @param bufferSize total size of the reading buffer
	 * @param readThreshold how much can be read before refilling the buffer
	 * @param readSize amount of bytes to read from the source each time
	 * @param readyThreshold minimum amount of available bytes to report isReady()
	 * @param notReadyThreshold maximum amount of available bytes to report isNotReady()
	 * @param waitForReady whether to wait for the buffer to be ready during reading
	 * @param endWithSource whether to end the streaming as soon as source returns 0 from read()
	 */
	BufferedStream(
		const std::string &taskName,
		size_t bufferSize,
		size_t readThreshold,
		size_t readSize,
		size_t readyThreshold,
		size_t notReadyThreshold,
		bool waitForReady = false,
		bool endWithSource = false);
	~BufferedStream() override;
	bool open(const std::shared_ptr<bell::ByteStream> &stream);
	void close() override;

	// inherited methods
  public:
	size_t read(uint8_t *dst, size_t len) override;
	size_t skip(size_t len) override;
	size_t position() override;
	size_t size() override;

	// stream status
  public:
	/**
	 * Total amount of bytes served to read().
	 */
	size_t readTotal;
	/**
	 * Amount of bytes available to read from the buffer.
	 */
	std::atomic<size_t> readAvailable;
	/**
	 * Whether the caller should start reading the data. This indicates that a safe
	 * amount (determined by readyThreshold) of data is available in the buffer.
	 */
	bool isReady() const;
	/**
	 * Whether the caller should stop reading the data. This indicates that the amount of data
	 * available for reading is decreasing to a non-safe value, as data is being read
	 * faster than it can be buffered.
	 */
	bool isNotReady() const;
	/**
	 * Semaphore that is given when the buffer becomes ready (isReady() == true). Caller can
	 * wait for the semaphore instead of continuously querying isReady().
	 */
	WrappedSemaphore readySem;

  private:
	bool running = false;
	bool terminate = false;
	WrappedSemaphore readSem; // signal to start writing to buffer after reading from it
	std::mutex readMutex;	  // mutex for locking read operations during writing, and vice versa
	size_t bufferSize;
	size_t readAt;
	size_t readSize;
	size_t readyThreshold;
	size_t notReadyThreshold;
	bool waitForReady;
	bool endWithSource;
	uint8_t *buf;
	uint8_t *bufEnd;
	uint8_t *bufReadPtr;
	uint8_t *bufWritePtr;
	std::shared_ptr<bell::ByteStream> source;
	void runTask() override;
	void reset();
	size_t lengthBetween(uint8_t *me, uint8_t *other);
};
