#ifndef GARLIC_STREAMS
#define GARLIC_STREAMS

#include <concepts>

namespace garlic {

  template<typename T> concept ReadableStream = requires(T t) {
    //! Read the current character from stream without moving the read cursor.
    { t.peek() } -> std::convertible_to<char>;
 
    //! Read the current character from stream and moving the read cursor to next character.
    { t.take() } -> std::convertible_to<char>;
 
    //! Get the current read cursor.
    //! \return Number of characters read from start.
    { t.tell() } -> std::convertible_to<std::size_t>;
  };

  template<typename T> concept WritableStream = requires(T t) {
    //! Begin writing operation at the current read pointer.
    //! \return The begin writer pointer.
    { t.put_begin() } -> std::convertible_to<char*>;
 
    //! Write a character.
    { t.put('a') };
 
    //! Flush the buffer.
    { t.flush() };
 
    //! End the writing operation.
    //! \param begin The begin write pointer returned by PutBegin().
    //! \return Number of characters written.
    { t.put_end("some string") } -> std::convertible_to<std::size_t>;
  };

}

#endif /* end of include guard: GARLIC_STREAMS */
