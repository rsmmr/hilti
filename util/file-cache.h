
#ifndef HILTI_UTIL_FILE_CACHE_H
#define HILTI_UTIL_FILE_CACHE_H

#include <istream>
#include <list>
#include <ostream>
#include <set>
#include <stdint.h>
#include <string>

namespace util {

namespace cache {

/// A class that caches already computed files on disk, with a number of
/// options to check if they are up to date with respect to a set of
/// dependencies.
class FileCache {
public:
    struct Key {
        typedef std::string Hash;

        /// A global scope to associate the content with. Keys with different
        /// scopes will never match.
        std::string scope;

        /// A name to associate with the key. This could, e.g., by the name
        /// of a module to be cahced.
        std::string name;

        /// A std::string representing a set of options to be associated with the
        /// content. This should probably be a hash. Keys with different
        /// option std::strings will never match.
        std::string options;

        /// A list of directories to associate with the content. Two keys
        /// with different directory lists will never match (order matters).
        std::set<std::string> dirs;

        /// A list of files to associate with the content. Two keys with
        /// different file lists will never match (order matters).
        /// Furthermore, if any of the files given here is newer than \a
        /// timestamp, the key will be considered invalid and not yield any
        /// matches.
        std::set<std::string> files;

        /// A list of hashes to associate with the context. Two keys with
        /// different hash lists will never match (order matters)..
        std::set<Hash> hashes;

        /// An internally managed timestamp that must be more recent than all
        /// dependencies for the key to be valid. A timestamp of zero
        /// disabled that check.
        time_t _timestamp = 0;

        // An internally managed field counting the number of data items
        // associated with the key.
        int _parts = 0;

        bool valid() const;
        void setInvalid();

        bool operator==(const Key& other) const;
        bool operator!=(const Key& other) const;

    private:
        bool _invalid = false;
    };

    /// Constructor.
    ///
    /// dir: Directory to store the cached content in. If it doesn't exist,
    /// it will be created. Existing content in the directory may be
    /// overwritten.
    FileCache(const std::string& dir);
    ~FileCache();

    /// Stores data under a given key.
    ///
    /// key: The key to identify the data.
    ///
    /// data: A list of data objects.
    bool store(const Key& key, std::list<std::string> data);

    /// Store data under a given key.
    ///
    /// data: A list of data objects to store, each with a pointer to
    /// beginning of the data and a size.
    ///
    /// Returns: True if the storage was successful.
    bool store(const Key& key, std::list<std::pair<const char*, size_t>> data);

    /// Store a single data object under a given key.
    ///
    /// data: Pointer to beginning of data.
    ///
    /// len: Length of data.
    ///
    /// Returns: True if the storage was successful.
    bool store(const Key& key, const char* data, size_t len);

    /// Looks up data under a given key. If the key is found and its content
    /// valid, returns the path to file containing the content.
    ///
    /// key: The key.
    ///
    /// Returns: A list of the data objects stored under the jey, or an empty list if not found.
    std::list<std::string> lookup(const Key& key);

private:
    std::string fileForKey(const Key& key, const std::string& prefix, int idx = 0);
    bool touchFile(const std::string& path, time_t time);

    std::string _dir;
};

/// Helper functions that returns the current time.
time_t currentTime();

/// Helper function that returns the time of a file's last modification.
///
/// path: The full path to the file.
time_t modificationTime(const std::string& path);

/// Helper function that hashes the contents of an input stream.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(std::istream& in);

/// Helper function that hashes the contents of a string.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(const std::string& str);

/// Helper function that hashes the contents of a memory block.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(const char* data, size_t len);

/// Helper function that hashes a size value.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(size_t size);
}
}

/// Outputs a key in an internal representation.
std::ostream& operator<<(std::ostream& out, const util::cache::FileCache::Key& key);

/// Inputs a key in an internal representation.
std::istream& operator>>(std::istream& in, util::cache::FileCache::Key& key);


#endif
