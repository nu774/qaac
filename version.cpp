const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.10";
#else
    return "1.02";
#endif
}
