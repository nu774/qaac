const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.42";
#else
    return "1.31";
#endif
}
