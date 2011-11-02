const char *get_qaac_version()
{
#ifdef NO_QT
    return "0.04";
#else
    return "0.95";
#endif
}
