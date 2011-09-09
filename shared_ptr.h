#ifdef __MINGW32__
#include <tr1/memory>
namespace x {
using std::tr1::shared_ptr;
}
#elif defined(_MSC_VER) && (_MSC_VER == 1500 && defined(_HAS_TR1) || _MSC_VER > 1500)
#include <memory>
namespace x {
using std::tr1::shared_ptr;
}
#else
#include <boost/shared_ptr.hpp>
namespace x {
using boost::shared_ptr;
}
#endif
