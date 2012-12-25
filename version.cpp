const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.10";
#else
    return "2.10";
#endif
}
