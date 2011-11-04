const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.05";
#else
    return "0.96";
#endif
}
