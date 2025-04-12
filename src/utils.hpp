#ifndef UTILS_H
#define UTILS_H

#define SUCCESS 0
#define FAIL    1

#define CHECK(cond, s, ret) {      \
  if ((cond)) {                    \
    std::cerr << (s) << std::endl; \
    if (errno) {                   \
      perror("");                  \
      errno = 0;                   \
    }                              \
    return (ret);                  \
  }                                \
}

#define CHECK_FREAD(file, fp, ret, size, fun_ret)                               \
  if ((ret) != (size)) {                                                        \
    std::cout << (file) << ": Failed reading " << (size)                        \
              << " objects. Only read: " << (ret) << " objects." << std::endl;  \
    if (errno) {                                                                \
      perror("");                                                               \
      errno = 0;                                                                \
    }                                                                           \
    if (fp != nullptr)                                                          \
      fclose(fp);                                                               \
    return (fun_ret);                                                           \
  }

#define CHECK_FWRITE(file, fp, ret, size, fun_ret)                              \
  if ((ret) != (size)) {                                                        \
    std::cout << (file) << ": Failed writing " << (size)                        \
              << " objects. Only wrote: " << (ret) << " objects." << std::endl; \
    if (errno) {                                                                \
      perror("");                                                               \
      errno = 0;                                                                \
    }                                                                           \
    if (fp != nullptr)                                                          \
      fclose(fp);                                                               \
    return (fun_ret);                                                           \
  }

#define ERR(s) {                                \
  (std::cout << "error: " << (s) << std::endl); \
  return 1;                                     \
}

#define DELETE_CNT(ptr)              \
  if ((ptr != nullptr)) {            \
    if ((ptr->content != nullptr)) { \
      delete (ptr->content);         \
      ptr->content = nullptr;        \
    }                                \
  }

#define DELETE_VEC(v) \
  for (size_t i = 0; i < v.size(); i++) { \
      delete v[i]; \
  } \
  v.clear()

#define DELETE_PTR(ptr) \
  if ((ptr != NULL)) {  \
    delete (ptr);       \
    ptr = nullptr;      \
  }

template<typename T1, typename T2>
struct Files {
    T1 path;
    T2 idx;

    Files(T1 x, T2 y) : path(x), idx(y) {}
};

#endif
