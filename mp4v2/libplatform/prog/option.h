#ifndef MP4V2_PLATFORM_PROG_OPTION_H
#define MP4V2_PLATFORM_PROG_OPTION_H

///////////////////////////////////////////////////////////////////////////////
///
/// @namespace mp4v2::platform::prog Command-line argument parsing.
/// <b>WARNING: THIS IS A PRIVATE NAMESPACE. NOT FOR PUBLIC CONSUMPTION.</b>
///
/// This namespace provides a mechanism to parse command-line arguments and
/// options for executables.
/// It is identical in behavior to <b>getopt_long</b> functions available
/// with many popular posix-platforms such as Darwin, FreeBSD and Linux.
/// Virtually any OS which has getopt_long will adequately document this
/// functionality. However, to avoid symbol ambiguity with the popular
/// posix implementation, the following identifiers have been renamed:
///     @li getopt_long()      -> getOption()
///     @li getopt_long_only() -> getOptionSingle()
///     @li option             -> Option
///     @li option.has_arg     -> Option.type
//!
///////////////////////////////////////////////////////////////////////////////
namespace mp4v2 { namespace platform { namespace prog {

//! On return from getOption() or getOptionSingle(),
//! points to an option argument, if it is anticipated.
MP4V2_EXPORT extern const char* optarg;

//! On return from getOption() or getOptionSingle(),
//! contains the index to the next argv argument for a subsequent call to
//! getOption() or getOptionSingle().
//! Initialized to 1 and must be set manually to 1 prior to invoking
//! getOption() or getOptionSingle() to evaluate multiple sets of arguments.
MP4V2_EXPORT extern int optind;

//! On return from getOption() or getOptionSingle(),
//! saves the last known option character returned by
//! getOption() or getOptionSingle().
//! On error, contains the character/code of option which caused error.
MP4V2_EXPORT extern int optopt;

//! Initialized to 1 and may be set to 0 to disable error messages.
MP4V2_EXPORT extern int opterr;

//! Must be set to 1 before evaluating the 2nd or each additional set
//! of arguments.
MP4V2_EXPORT extern int optreset;

//! Structure describing a single option.
//! An instance of Option is required for each program option and is
//! initialized before use with getOption() or getOptionWord().
struct MP4V2_EXPORT Option
{
    //! expectation-type indicating number of arguments expected
    //! on the command-line following the option-argument itself
    enum Type {
        //! indicates exactly 0 arguments follow option
        NO_ARG,
        //! indicates exactly 1 argument follow option
        REQUIRED_ARG,
        //! indicates 0 or 1 arguments follow option
        OPTIONAL_ARG,
    };

    //! contains the option name without leading double-dash
    const char* name;

    //! option expectation-type
    Type type;

    //! If not NULL, then the integer pointed to by it will be set to
    //! the value in the val field. If the flag field is NULL, then the
    //! <b>val</b> field will be returned.
    int* flag;

    //! Constant value representing option. This is usually a single-char
    //! ASCII value but in case of long-options without a corresponding
    //! single-char value it can be a unique integer (beyond ASCII range)
    //! which represents the long-option.
    int val;
};

///////////////////////////////////////////////////////////////////////////////
//!
//! Get option character from command line argument list.
//!
//! getOption() is similar to posix getopt() but it accepts options in two
//! forms: words and characters. The getOption() function provides a
//! superset of the functionality of getopt(). The getOption() function can
//! be used in two ways. In the first way, every long-option understood by
//! the program has a corresponding short-option, and the Option structure
//! is only used to translate from long-options to short-options. When used
//! in this fashion, getOption() behaves identically to getopt(). This is
//! a good way to add long-option processing to an esxisting program with
//! a minimum of rewriting.
//!
//! In the second mechanism, a long-option sets a flag in the Option
//! structure structure passed, or will store a pointer to the command line
//! argument in the Option structure passed to it for options that take
//! arguments. Additionally, the long-option's argument may be specified as
//! a single argument with an equal sign, eg:
//!     @code myprogram --myoption=somevalue
//!     @endcode
//!
//! When a long-option is processed, the call to getOption() will return 0.
//! For this reason, long-option processing without shortcuts is not
//! backwards compatible with getopt().
//!
//! It is possible to combine these methods, providing for long-options
//! processing with short-option equivalents for some options.  Less
//! frequently used options would be processed as long-options only.
//!
//! @param argc number of arguments.
//! @param argv argument array of strings.
//! @param optstr string containing the following elements:
//!     individual characters, and characters followed by a colon to indicate
//!     an option argument is to follow. For example, an option string "x"
//!     recognizes an option "-x", and an option string "x:" recognizes an
//!     option and argument "-x argument".
//! @param longopts array of Option entries. The last element must be filled
//!     with zeroes to indicate end-of-array.
//! @param idx If not NULL, then the integer pointed to it will be set to
//!     the index of the long-option relative to <b>longops</b>.
//!
//! @return If the <b>flag</b> field is NULL, <b>val</b> field is returned,
//!     which is usually just the corresponding short-option.
//!     If <b>flag</b> is not NULL, 0 is returned and <b>val</b> is
//!     stored in the location pointed to by <b>flag</b> field.
//!     A ':' will be returned if there was a missing option argument.
//!     A '?' will be returned if an unknown or ambiguous option was used.
//!     A -1 will be returned when the argument list has been exhausted.
//!
///////////////////////////////////////////////////////////////////////////////
MP4V2_EXPORT
int getOption( int argc, char* const* argv, const char* optstr, const Option* longopts, int* idx );

///////////////////////////////////////////////////////////////////////////////
//!
//! Get option character from command line argument list and allow
//! long-options with single-hyphens.
//!
//! Behaves identically to getOption() with the exception that long-options
//! may start with '-' in addition to '--'.
//! If an option starting with '-' does not match a long option but does match
//! a single-character option, the single-character option is returned.
//!
//! @param argc number of arguments.
//! @param argv argument array of strings.
//! @param optstr string containing the following elements:
//!     individual characters, and characters followed by a colon to indicate
//!     an option argument is to follow. For example, an option string "x"
//!     recognizes an option "-x", and an option string "x:" recognizes an
//!     option and argument "-x argument".
//! @param longopts array of Option entries. The last element must be filled
//!     with zeroes to indicate end-of-array.
//! @param idx If not NULL, then the integer pointed to it will be set to
//!     the index of the long-option relative to <b>longops</b>.
//!
//! @return If the <b>flag</b> field is NULL, <b>val</b> field is returned,
//!     which is usually just the corresponding short-option.
//!     If <b>flag</b> is not NULL, 0 is returned and <b>val</b> is
//!     stored in the location pointed to by <b>flag</b> field.
//!     A ':' will be returned if there was a missing option argument.
//!     A '?' will be returned if an unknown or ambiguous option was used.
//!     A -1 will be returned when the argument list has been exhausted.
//!
///////////////////////////////////////////////////////////////////////////////
MP4V2_EXPORT
int getOptionSingle( int argc, char* const* argv, const char* optstr, const Option* longopts, int* idx );

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::prog

#endif // MP4V2_PLATFORM_PROG_OPTION_H
