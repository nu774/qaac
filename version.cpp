const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.16";
#else
    return "2.16";
#endif
}
