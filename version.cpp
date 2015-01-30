const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.45";
#else
    return "2.45";
#endif
}
