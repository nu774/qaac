const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.02";
#else
    return "0.94";
#endif
}
