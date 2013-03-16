
#ifndef HILTI_UTIL_FILE_CACHE_H
#define HILTI_UTIL_FILE_CACHE_H

#include <ostream>
#include <istream>
#include <list>
#include <string>
#include <stdint.h>

namespace util {

namespace cache {

/// A class that caches already computed files on disk, with a number of
/// options to check if they are up to date with respect to a set of
/// dependencies.
class FileCache
{
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
        std::list<std::string> dirs;

        /// A list of files to associate with the content. Two keys with
        /// different file lists will never match (order matters).
        /// Furthermore, if any of the files given here is newer than \a
        /// timestamp, the key will be considered invalid and not yield any
        /// matches.
        std::list<std::string> files;

        /// A list of hashes to associate with the context. Two keys with
        /// different hash lists will never match (order matters)..
        std::list<Hash> hashes;

        /// An internally managed timestamp that must be more recent than all
        /// dependencies for the key to be valid. A timestamp of zero
        /// disabled that check.
        time_t _timestamp;

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

    /// Stores the data under a given key.
    ///
    /// key: The key to identify the data.
    ///
    /// data: The data itself.
    bool store(const Key& key, std::istream& data);

    /// Stores the data under a given key.
    ///
    /// key: The key to identify the data.
    ///
    /// data: The data itself.
    bool store(const Key& key, const std::string& data);

    /// Looks up data under a given key. If the key is found and its content
    /// valid, returns the path to file containing the content.
    ///
    /// key: The key.
    ///
    /// Returns: The path to the content, or an empty string if not found.
    std::string lookup(const Key& key);

    /// Looks up data under a given key. If the key is found and its content
    /// valid, the cached data is saved into \a data.
    ///
    /// key: The key.
    ///
    /// data: Where to store the cached content if valid.
    ///
    /// Returns: True if cached content is found an valid. In that case, \a
    /// data will be filled accordingly.
    bool lookup(const Key& key, std::ostream& data);

    /// Looks up data under a given key. If the key is found and its content
    /// valid, the cached data is saved into \a data.
    ///
    /// key: The key.
    ///
    /// data: Where to store the cached content if valid.
    ///
    /// Returns: True if cached content is found an valid. In that case, \a
    /// data will be filled accordingly.
    bool lookup(const Key& key, std::string* data);

private:
    std::string fileForKey(const Key& key);
    bool touchFile(const std::string& path, time_t time);

    std::string _dir;

};

/// Helper functions that returns the current time.
time_t currentTime();

/// Helper function that returns the time of a file's last modification.
///
/// path: The full path to the file.
time_t modificationTime(const std::string& path);

/// Helper function that hashes the contents of an input stream into an integer.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(std::istream& in);

/// Helper function that hashes the contents of a file into an integer.
///
/// Returns a hash on success, or "" if there's an error.
FileCache::Key::Hash hash(const std::string& str);

}

}

/// Outputs a key in an internal representation.
std::ostream& operator<<(std::ostream &out, const util::cache::FileCache::Key& key);

/// Inputs a key in an internal representation.
std::istream& operator>>(std::istream &in,  util::cache::FileCache::Key& key);


#endif

