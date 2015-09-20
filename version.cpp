const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.53";
#else
    return "2.53";
#endif
}
