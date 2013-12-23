const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.32";
#else
    return "2.32";
#endif
}
