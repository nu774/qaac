#ifndef MP4V2_PLATFORM_IO_FILESYSTEM_H
#define MP4V2_PLATFORM_IO_FILESYSTEM_H

namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////
///
/// General file-system abstraction.
///
/// FileSystem abstracts operations on files and directories.
///
///////////////////////////////////////////////////////////////////////////////
class MP4V2_EXPORT FileSystem
{
public:
    static string DIR_SEPARATOR;  //!< separator string used in file pathnames
    static string PATH_SEPARATOR; //!< separator string used in search-paths

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Query file presence.
    //! Check if <b>name</b> exists.
    //! @param name filename to query.
	//!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //! @return true if present, false otherwise.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static bool exists( std::string name );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Query directory type.
    //! Check if <b>name</b> exists and is a directory.
    //! @param name pathname to query.
	//!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //! @return true if directory, false otherwise.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static bool isDirectory( std::string name );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Query file type.
    //! Check if <b>name</b> exists and is a file.
    //!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //! @param name filename to query.
    //! @return true if file, false otherwise.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static bool isFile( std::string name );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Query file size.
    //! Check if <b>name</b> exists and is a file.
    //! @param name filename to query.
    //!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //! @param size output indicating file size in bytes.
    //! @return true on failure, false on success.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static bool getFileSize( std::string name, File::Size& size );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Rename file or directory.
    //!
    //! Rename <b>oldname</b> to <b>newname</b>.
    //! If <b>newname</b> exists, it is first removed.
    //! Both <b>oldname</b> and <b>newname</b> must be of the same type;
    //! that is, both must be either files or directories and must reside on
    //! the same filesystem.
    //!
    //! @param oldname existing pathname to rename.
    //!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //! @param newname new pathname.
    //!     On Windows, this should be a UTF-8 encoded string.
    //!     On other platforms, it should be an 8-bit encoding that is
    //!     appropriate for the platform, locale, file system, etc.
    //!     (prefer to use UTF-8 when possible).
    //!
    //! @return true on failure, false on success.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static bool rename( std::string oldname, std::string newname );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Generate temporary pathname.
    //!
    //! @param name output containing generated pathname.
    //! @param dir relative or absolute directory of pathname.
    //! @param prefix text prepended to base pathname.
    //! @param suffix text appended to base pathname.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static void pathnameTemp( string& name, string dir = ".", string prefix = "tmp", string suffix = "" );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Cleanup pathname.
    //!
    //! Redundant (repeating) directory-separators are folded into a single
    //! directory-separator.
    //!
    //! Redundant /./ are folded into a single directory-separator.
    //!
    //! @param name pathname to modify.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static void pathnameCleanup( string& name );

#if 0
TODO-KB: implement
    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Remove everything after the last directory component.
    //!
    //! A pathname cleanup is always performed. See pathnameCleanup().
    //! If no directory component is present then "." is assumed.
    //!
    //! @param name pathname to modify.
    //! @param trailing when true all results are suffixed with exactly one
    //!     directory-separator, otherwise the result is guaranteed to not
    //!     end in a directory-separator.
    //!
    ///////////////////////////////////////////////////////////////////////////
    static void pathnameOnlyDirectory( string& name, bool trailing = true );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Remove everything except the file component of pathname.
    //!
    //! A pathname cleanup is always performed. See pathnameCleanup().
    //! If no file component exists then an empty-string is output.
    //! A file component may include an extension.
    //!
    //! @param name pathname to modify.
    //!
    ///////////////////////////////////////////////////////////////////////////
    static void pathnameOnlyFile( string& name );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Remove everything except file basename.
    //!
    //! A pathname cleanup is always performed. See pathnameCleanup().
    //! A basename is considered to be everything before the last '.'
    //! in the file component of a pathname.
    //! If no file extension exists then an empty-string is output.
    //!
    //! @param name pathname to modify.
    //!
    ///////////////////////////////////////////////////////////////////////////
    static void pathnameOnlyBasename( string& name );
#endif

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Remove everything except file extension.
    //!
    //! A pathname cleanup is always performed. See pathnameCleanup().
    //! A file extension is considered to everything <b>after</b>
    //! the last '.' in the file component of a pathname.
    //! If no file extension exists then an empty-string is output.
    //!
    //! @param name pathname to modify.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static void pathnameOnlyExtension( string& name );

    ///////////////////////////////////////////////////////////////////////////
    //!
    //! Remove file extension from pathname.
    //!
    //! A pathname cleanup is always performed. See pathnameCleanup().
    //! A file extension is considered to everything <b>after</b>
    //! the last '.' in the file component of a pathname.
    //! The last '.' is also removed.
    //!
    //! @param name pathname to modify.
    //!
    ///////////////////////////////////////////////////////////////////////////

    static void pathnameStripExtension( string& name );
};

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io

#endif // MP4V2_PLATFORM_IO_FILESYSTEM_H
