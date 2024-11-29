#ifndef _MINIATURA_BASEDEFS_H_
#define _MINIATURA_BASEDEFS_H_

#if MINIATURA_DYNAMIC_LINK
	#if defined(_MSC_VER)
		#define MINIATURA_DLLEXPORT __declspec(dllexport)
		#define MINIATURA_DLLIMPORT __declspec(dllimport)
	#elif defined(__GNUC__) || defined(__clang__)
		#define MINIATURA_DLLEXPORT __visbility__((__default__))
		#define MINIATURA_DLLIMPORT __visbility__((__default__))
	#endif
#else
	#define MINIATURA_DLLEXPORT
	#define MINIATURA_DLLIMPORT
#endif

#if defined(_MSC_VER)
	#define MINIATURA_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define MINIATURA_FORCEINLINE __attribute__((__always_inline__)) inline
#endif

#if defined(_MSC_VER)
	#define MINIATURA_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier extern template class name<__VA_ARGS__>;
	#define MINIATURA_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		apiModifier template class name<__VA_ARGS__>;
#elif defined(__GNUC__) || defined(__clang__)
	#define MINIATURA_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		extern template class apiModifier name<__VA_ARGS__>;
	#define MINIATURA_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...) \
		template class name<__VA_ARGS__>;
#else
	#define MINIATURA_DECL_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
	#define MINIATURA_DEF_EXPLICIT_INSTANTIATED_CLASS(apiModifier, name, ...)
#endif

#if IS_MINIATURA_BUILDING
	#define MINIATURA_API MINIATURA_DLLEXPORT
#else
	#define MINIATURA_API MINIATURA_DLLIMPORT
#endif

namespace peff {
}

#endif
