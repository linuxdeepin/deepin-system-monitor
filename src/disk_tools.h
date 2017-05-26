#ifndef DISKTOOLS_H
#define DISKTOOLS_H

namespace diskTools {
    typedef struct DiskStatus {
        float readKbs;
        float writeKbs;
    } DiskStatus;
    
    /**
     * Process IO details from /proc/pid/io. Note that the IO counters encompass all IO, not only to disk, but also pipes and sockets.
     * Root access is required to read /prod/pid/io (one may detect the size of a password of another process by examining this file),
     */
    struct ProcPidIO {
      /**
       * characters read - The number of bytes which this task has caused to be read from storage. This is simply the sum of bytes which this process passed to
       * read(2) and similar system calls. It includes things such as terminal I/O and is unaffected by whether or not actual physical disk I/O
       * was required (the read might have been satisfied from pagecache).
       */
      unsigned long rchar;
      /**
       * characters written - The number of bytes which this task has caused, or shall cause to be written to disk.  Similar caveats apply here as with rchar.
       */
      unsigned long wchar;
      /**
       * read syscalls - Attempt to count the number of read I/O operations—that is, system calls such as read(2) and pread(2).
       */
      unsigned long syscr;
      /**
       * write syscalls - Attempt to count the number of write I/O operations—that is, system calls such as write(2) and pwrite(2).
       */
      unsigned long syscw;
      /**
       * bytes read - Attempt to count the number of bytes which this process really did cause to be fetched from the  storage  layer.   This  is  accurate  for
       * block-backed filesystems.
       */
      unsigned long read_bytes;
      /**
       * bytes written - Attempt to count the number of bytes which this process caused to be sent to the storage layer.
       */
      unsigned long write_bytes;
      /**
       * The  big  inaccuracy  here is truncate.  If a process writes 1MB to a file and then deletes the file, it will in fact perform no writeout.
       * But it will have been accounted as having caused 1MB of write.  In other words: this field represents  the  number  of  bytes  which  this
       * process caused to not happen, by truncating pagecache.  A task can cause "negative" I/O too.  If this task truncates some dirty pagecache,
       * some I/O which another task has been accounted for (in its write_bytes) will not be happening.
       */
      unsigned long cancelled_write_bytes;
    };
        
    /**
     * Get process IO details, root access required.
     * @param pid the process id.
     * @param io the ProcPidIO struct to fill.
     * @return false if the operation failed (no permission, pid does not exist).
     * @root root access is required even for processes the caller owns (as the information in this file
     * might be used to deduce the size of an entered password).
     */
    bool getProcPidIO(int pid, ProcPidIO &io );
}

#endif // DISKTOOLS_H
