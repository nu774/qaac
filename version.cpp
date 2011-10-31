const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.01";
#else
    return "0.94";
#endif
}
