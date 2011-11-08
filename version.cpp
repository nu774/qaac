const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.07";
#else
    return "0.98";
#endif
}
