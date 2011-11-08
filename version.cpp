const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.06";
#else
    return "0.97";
#endif
}
