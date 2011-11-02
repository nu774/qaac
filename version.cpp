const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.03";
#else
    return "0.95";
#endif
}
