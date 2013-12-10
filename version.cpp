const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.28";
#else
    return "2.28";
#endif
}
