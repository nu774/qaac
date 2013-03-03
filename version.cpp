const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.17";
#else
    return "2.17";
#endif
}
